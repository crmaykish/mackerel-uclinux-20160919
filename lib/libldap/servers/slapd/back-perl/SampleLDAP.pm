# This is a sample Perl module for the OpenLDAP server slapd.
#
# $OpenLDAP: pkg/ldap/servers/slapd/back-perl/SampleLDAP.pm,v 1.3.2.2 2002/05/22 14:25:55 kurt Exp $
#
# Usage: Add something this to slapd.conf:
#
#	database	perl
#	suffix		"o=AnyOrg, c=US"
#	perlModulePath	/path/to/this/file
#	perlModule	SampleLDAP

package SampleLDAP;

use POSIX;

sub new
{
	my $class = shift;

	my $this = {};
	bless $this, $class;
        print STDERR "Here in new\n";
	print STDERR "Posix Var " . BUFSIZ . " and " . FILENAME_MAX . "\n";
	return $this;
}

sub init
{
	return 0;
}

sub search
{
	my $this = shift;
	my($base, $scope, $deref, $sizeLim, $timeLim, $filterStr, $attrOnly, @attrs ) = @_;
        print STDERR "====$filterStr====\n";
	$filterStr =~ s/\(|\)//g;
	$filterStr =~ s/=/: /;

	my @match_dn = ();
	foreach my $dn ( keys %$this ) {
		if ( $this->{ $dn } =~ /$filterStr/im ) {
			push @match_dn, $dn;
			last if ( scalar @match_dn == $sizeLim );

		}
	}

	my @match_entries = ();
	
	foreach my $dn ( @match_dn )  {
		push @match_entries, $this->{ $dn };
	}

	return ( 0 , @match_entries );

}

sub compare
{
	my $this = shift;
	my ( $dn, $avaStr ) = @_;
	my $rc = 5; # LDAP_COMPARE_FALSE

	$avaStr =~ s/=/: /;

	if ( $this->{ $dn } =~ /$avaStr/im ) {
		$rc = 6; # LDAP_COMPARE_TRUE
	}

	return $rc;
}

sub modify
{
	my $this = shift;

	my ( $dn, @list ) = @_;

	while ( @list > 0 ) {
		my $action = shift @list;
		my $key    = shift @list;
		my $value  = shift @list;

		if( $action eq "ADD" ) {
			$this->{ $dn } .= "$key: $value\n";

		}
		elsif( $action eq "DELETE" ) {
			$this->{ $dn } =~ s/^$key:\s*$value\n//mi ;

		}
		elsif( $action eq "REPLACE" ) {
			$this->{ $dn } =~ s/$key: .*$/$key: $value/im ;
		}
	}

	return 0;
}

sub add
{
	my $this = shift;

	my ( $entryStr ) = @_;

	my ( $dn ) = ( $entryStr =~ /dn:\s(.*)$/m );

	#
	# This needs to be here until a normalized dn is
	# passed to this routine.
	#
	$dn = uc( $dn );
	$dn =~ s/\s*//g;


	$this->{$dn} = $entryStr;

	return 0;
}

sub modrdn
{
	my $this = shift;

	my ( $dn, $newdn, $delFlag ) = @_;

	$this->{ $newdn } = $this->{ $dn };

	if( $delFlag ) {
		delete $this->{ $dn };
	}
	return 0;

}

sub delete
{
	my $this = shift;

	my ( $dn ) = @_;
	
        print STDERR "XXXXXX $dn XXXXXXX\n";
	delete $this->{$dn};
}

sub config
{
	my $this = shift;

	my ( @args ) = @_;
        local $, = " - ";
        print STDERR @args;
        print STDERR "\n";
	return 0;
}

1;
