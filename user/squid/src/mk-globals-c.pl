print "#include \"squid.h\"\n";
while (<>) {
	$init = undef;
	next if (/ SQUID_GLOBALS_H/);
	if (/^#/) {
		print;
		next;
	}
	if (/^.\*/) {
		print;
		next;
	}
	next unless (/./);
	next if (/\[\];$/);
	die unless (/^extern\s+([^;]+);(.*)$/);
	$var = $1;
	$comments = $2;
	if ($comments =~ m+/\*\s*(.*)\s*\*/+) {
		$init = $1;
		$init =~ s/\s$// while ($init =~ /\s$/);
	}
	print $var;
	print " = $init" if (defined $init);
	print ";\n";
}
exit 0;
