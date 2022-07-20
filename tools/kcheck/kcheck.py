# Prints a config that includes only the symbols that are visible.

import kconfiglib
import os
import sys

class Config(object):
    def __init__(self, products, config_dir):
        # The kconfiglib config objects, one for each section
        self.config = {}
        # The directory for each .config file
        self.config_file = {}
        # Flat dict of the items for each config object
        self.item = {}

        try:
            self.rootdir = os.environ['ROOTDIR']
        except KeyError:
            self.rootdir = os.getcwd()

        config_dir = os.path.abspath(config_dir)
        if config_dir is None or config_dir == self.rootdir:
            self.config_file['device'] = self.rootdir + '/.config'
            self.config_file['linux'] = self.rootdir + '/linux/.config'
            self.config_file['modules'] = self.rootdir + '/modules/.config'
            self.config_file['libc'] = self.rootdir + '/uClibc/.config'
            self.config_file['user'] = self.rootdir + '/config/.config'
        else:
            self.config_file['device'] = config_dir + '/config.device'
            self.config_file['linux'] = config_dir + '/config.linux'
            self.config_file['modules'] = config_dir + '/config.modules'
            self.config_file['libc'] = config_dir + '/config.uClibc'
            self.config_file['user'] = config_dir + '/config.vendor'

        self.load_config('device', '/Kconfig', '', True)

        self.product = self.find_product(products)
        os.environ['ARCH'] = self.product['arch']
        os.environ['SRCARCH'] = self.product['arch']
        os.environ['KERNELVERSION'] = 'unknown'

        self.load_config('linux', '/linux/Kconfig', '/linux', False)
        self.load_config('modules', '/modules/Kconfig', '/modules', False)

        # uClibc's Kconfig isn't compatible; it doesn't include CONFIG_
        if False:
            if self.check_value('device', 'DEFAULTS_LIBC_UCLIBC'):
                load_config('libc', '/uClibc/extra/Configs/Config.in', '/uClibc', False)
            else:
                raise KeyError('unsupported libc')

        self.load_config('user', '/config/Kconfig', '/config', True)

    def find_product(self, products):
        for product in products.values():
            name = configify('DEFAULTS_' + product['vendor'] + '_' + product['product'])
            if self.check_value('device', name):
                return product
        raise KeyError('product not found')

    def load_config(self, section, kconfig, base_dir, uclinux_dist):
        self.config[section] = kconfiglib.Config(self.rootdir + kconfig, base_dir=self.rootdir + base_dir);
        self.config[section].set_uclinux_dist(uclinux_dist)
        self.config[section].load_config(self.config_file[section])
        self.item[section] = {}
        self.load_items(self.item[section], self.get_config_items(section))

    def load_items(self, items, config_items):
        for item in config_items:
            if item.is_menu():
                self.load_items(items, item.get_items())
            elif item.is_choice():
                self.load_items(items, item.get_items())
            elif item.is_symbol():
                items[item.get_name()] = item

    def get_sections(self):
        return self.config.keys()

    def get_section(self, section):
        return self.config[section]

    def get_config_items(self, section):
        return self.config[section].get_top_level_items()

    def get_items(self, section):
        return self.item[section]

    def get_item(self, section, name):
        return self.item[section][name]

    def get_value(self, section, name):
        return self.item[section][name].get_value()

    def check_value(self, section, name, val='y'):
        try:
            return self.get_value(section, name) == val
        except KeyError:
            return False

    def write(self):
        for section in self.get_sections():
            self.config[section].write_config(self.config_file[section],
                    self.config[section].get_config_header())
        with open(self.config_file['device'], 'a') as f:
            f.write('CONFIG_VENDOR=%s\n' % self.product['vendor'])
            f.write('CONFIG_PRODUCT=%s\n' % self.product['product'])
            f.write('CONFIG_LINUXDIR=linux\n')
            f.write('CONFIG_LIBCDIR=uClibc\n')

    # Test code
    def write_visible(self):
        for section in self.get_sections():
            self.write_visible_items(section)

    def write_visible_items(self, section, noconfig=False):
        f = open(self.config_file[section], 'w')
        for name, item in self.item[section].items():
            if item.is_modifiable() and item.get_value() != 'n':
                item.already_written = False
                val = item._make_conf()[0]
                if noconfig:
                    val = val.replace('CONFIG_', '', 1)
                print >>f, val
        f.close()

    # Test code
    def display(self):
        for items in self.item.values():
            self.display_items(items)

    def display_items(self, items):
        for name, item in items.items():
            if item.is_modifiable() and item.get_value() != 'n':
                item.already_written = False
                print item._make_conf()[0]

def configify(name):
    return name.upper().replace('-', '_')

class Product(object):
    def __init__(self, config, products, groups):
        self.error = False
        self.sections = config.get_sections()
        self.load(config.product, groups)

    def load(self, product, groups):
        self.item = {}
        for section in self.sections:
            self.item[section] = {}

        self.load_product(product)

        todo = set(product['include'])
        done = set()
        while len(todo):
            name = todo.pop()
            if name in done:
                continue
            group = groups[name]
            if 'include' in group:
                todo |= set(group['include'])
            self.load_group(group)
            done.add(name)

    def load_product(self, product):
        self.item['device'][configify('DEFAULTS_' + product['vendor'])] = ('y', [])
        self.item['device'][configify('DEFAULTS_' + product['vendor'] + '_' + product['product'])] = ('y', [])
        self.item['device'][configify('DEFAULTS_LIBC_UCLIBC')] = ('y', [])
        self.load_group(product)

    def load_group(self, group):
        for section in self.sections:
            if section in group:
                self.load_section(group, section)

    def load_section(self, group, section):
        for name, val in group[section].items():
            try:
                prevval, groups = self.item[section][name]
                if not isinstance(prevval, list):
                    prevval = [ prevval ]
                if not isinstance(val, list):
                    val = [ val ]
                newval = self.merge_item(list(prevval), list(val))
                assert len(newval), (
                        'load mismatch:\n%s=%s in %s\n%s=%s in %s'
                        % (name, ', '.join(prevval), ', '.join(groups),
                           name, ', '.join(val), group['name']))
                if len(newval) == 1:
                    newval = newval[0]
                groups.append(group['name'])
                self.item[section][name] = (newval, groups)
            except KeyError:
                self.item[section][name] = (val, [group['name']])

    def merge_item(self, list1, list2):
        common = set(list1).intersection(list2)
        item = []
        while list1 and list2:
            if list1[0] not in common:
                list1.pop(0)
            elif list2[0] not in common:
                list2.pop(0)
            else:
                if list1[0] != list2[0]:
                    return []
                item.append(list1[0])
                list1.pop(0)
                list2.pop(0)
        while list1:
            if list1[0] in common:
                item.append(list1[0])
            list1.pop(0)
        while list2:
            if list2[0] in common:
                item.append(list2[0])
            list1.pop(0)
        return item

    def check(self, config):
        self.mismatch = {}
        self.missing = {}
        self.unknown = {}
        self.unneeded = {}
        self.error = False
        for section in self.sections:
            self.mismatch[section] = {}
            self.missing[section] = {}
            self.unknown[section] = {}
            self.unneeded[section] = {}
            self.check_section(section, config.get_items(section))
        for section in self.sections:
            self.display_errors(section, config.get_config_items(section))
            for name, (val, groups) in self.missing[section].items():
                print 'missing: %s: %s=%s, groups %s' % (section, name, val, groups)
                self.error = True
        return self.error

    def check_section(self, section, items):
        for name, (val, groups) in self.item[section].items():
            if isinstance(val, list):
                val = val[0]
            try:
                item = items[name]
                if item.get_value() != str(val):
                    self.mismatch[section][name] = (val, groups)
                elif not item.is_modifiable() or item.get_value() == 'n':
                    self.unneeded[section][name] = (val, groups)
            except KeyError:
                self.missing[section][name] = (val, groups)
        for name, item in items.items():
            if item.is_modifiable() and item.get_value() != 'n' and name not in self.item[section]:
                self.unknown[section][name] = True

    def display_errors(self, section, items):
        for item in items:
            if item.is_menu():
                self.display_errors(section, item.get_items())
            elif item.is_choice():
                self.display_errors(section, item.get_items())
            elif item.is_symbol():
                name = item.get_name()
                try:
                    val, groups = self.mismatch[section][name]
                    print ('mismatch: %s: %s=%s, expected %s, groups %s'
                            % (section, name, item.get_value(), val, groups))
                    self.error = True
                except KeyError:
                    pass

                try:
                    val = self.unknown[section][name]
                    print ('unknown: %s: %s=%s'
                            % (section, name, item.get_value()))
                    self.error = True
                except KeyError:
                    pass

                try:
                    val = self.unneeded[section][name]
                    #print ('unneeded: %s: %s=%s' % (section, name, item.get_value()))
                    self.error = True
                except KeyError:
                    pass

    def set(self, config):
        for section in self.sections:
            config.get_section(section).unset_user_values()
            self.set_section(section, config.get_config_items(section))
        config.write()

    def set_section(self, section, items):
        for item in items:
            if item.is_menu():
                self.set_section(section, item.get_items())
            elif item.is_choice():
                self.set_section(section, item.get_items())
            elif item.is_symbol():
                name = item.get_name()
                try:
                    val, groups = self.item[section][name]
                    if isinstance(val, list):
                        val = val[0]
                    item.set_user_value(str(val))
                except KeyError:
                    if item.prompts != [] and item.get_type() in (kconfiglib.BOOL, kconfiglib.TRISTATE):
                        item.set_user_value('n')
