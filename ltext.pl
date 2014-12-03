#!/usr/bin/perl

$ldir = "install-files/share/wcm/lang";

opendir($dh, ".") || die;
while(readdir $dh) {
	if (/(.*\.cpp)/ || /(.*\.h)/) {
		$file_name = $1;
		open (F, $file_name) || die;
		$ln = 1;
		while (<F>)
		{
			while (m/_LT\(\s*("[^\"]*")/g) 
			{
				push @{$a{$1}}, "$file_name:$ln";
			}
			$ln++;
		}	
		close(F);
	}
}
closedir $dh;

 
opendir($dh, "$ldir") || die;
while(readdir $dh) {
	undef  %z;
	next if (!/ltext.*/);
	next if (/.*\.new/);
	
	next if (/.*\.o/);
	next if (/.*\.cpp/);
	next if (/.*\.h/);
	
undef  %x;
	
	$fname = $_;
	
	open (F, $ldir."/".$fname) || die;
	while (<F>) 
	{
		if (/^\s*id\s*("[^\"]*").*/){
			$id = $1;
		}
		if (/^(txt\s+.*)$/){
			undef  @txt;
			push @txt, $1;
		}
		if (/"[^\"]*"/) {
			push @txt, $_;
		}
		if (/\s*\#/) {
			next;
		} 
		if (/^\s*$/){
			@x{$id} = @txt;
		}
	};
	close(F);
	
	open (F, ">",$ldir."/".$fname.".new") || die;
	
	foreach $k (sort keys %a) {
		foreach $s (@{$a{$k}})
		{
			print F "#$s\n";
		};
		print F "id $k\n";
		if (exists($x{$k}))
		{
			@t = @x{$k};
			foreach $c (@t){
				print F "$c\n";
			}
		} else {
			print F "txt \"\"\n";
		}
		print F "\n";
	};
	close F;
}

