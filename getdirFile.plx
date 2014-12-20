#!/usr/bin/perl -w
my @files = `ls src`;
my $rootdir;
foreach(@files) {
	chomp;
	if(/planner-(\S+)\.c/) {
		my($class) = /planner-(\S+)\.c/;
		@class = split '-',$class;
		print $_." " foreach(@class);
		$temp;
		foreach(reverse @class) {
			$rootdir_temp = $rootdir_temp->{"$_"};
		}
		$rootdir_temp = 1;
		print "\n";
	}
}
print $_.'..' foreach keys(%{$rootdir});
