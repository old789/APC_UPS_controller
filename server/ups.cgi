#!/usr/local/bin/perl

use strict;
use POSIX;
use Sys::Syslog qw(:DEFAULT setlogsock);
use Fcntl ':flock';

my $log_file_name='/home/user/ups_stat/ups_stat_'.POSIX::strftime("%d_%m_%Y",localtime()).'.log';

my $name='';
my $value='';
my $pair='';
my %form=();
my $i=0;

my $req_metod=exists($ENV{'REQUEST_METHOD'})?$ENV{'REQUEST_METHOD'}:"none";
my $rem_addr=exists($ENV{'REMOTE_ADDR'})?$ENV{'REMOTE_ADDR'}:"";

if ($req_metod ne 'POST') {
  &mylogger('HTTP query with REQUEST_METHOD = '.$req_metod.($rem_addr?' from IP '.$rem_addr:''));
  &byebye("Wrong format of input data\n");
}

my $post_query_string=<STDIN>;
chomp($post_query_string);

my @pairs=split(/&/, $post_query_string);
foreach $pair (@pairs){
  $pair =~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C", hex($1))/eg;
  if (($i=index($pair,"=")) < 1) { &mylogger(" - strange pair: ".$pair.($rem_addr?' ( IP '.$rem_addr.')':'')); next; }
  $name=substr($pair,0,$i);
  $value=substr($pair,$i+1);
  $form{$name}=$value;
}

write2log();

&byebye("OK\n");
exit;

sub mylogger{
 setlogsock("unix");
 openlog("pwr_stat", "nowait", "user");
 syslog("err",join(" ",@_));
 closelog;
 return;
}

sub byebye{
 my $outmesg=shift;

 print "Content-type: text/plain\n\n".$outmesg;
 exit;
}

sub write2log{
my $try=0;;
my $wr_str='';

  $wr_str=POSIX::strftime("%d-%m-%Y %H:%M:%S",localtime()).' '.
          (exists($form{'name'})?$form{'name'}:'unknown').' '.
          (exists($form{'model'})?'"'.$form{'model'}.'"':'"generic UPS"').' '.
          (exists($form{'uptime'})?$form{'uptime'}:'nouptime').' ';
  if (exists($form{'data'})) {
    $wr_str.='data="'.$form{'data'}.'"';
  }elsif (exists($form{'msg'})) {
    $wr_str.='message="'.$form{'msg'}.'"';
  }else{
    $wr_str.='incorrect record';
  }

  for ($try=0;$try<3;$try++){
    if (open(oLOG,">>".$log_file_name)) {
      unless (flock(oLOG,LOCK_EX|LOCK_NB)){
        close(oLOG);
      }else{
        print oLOG $wr_str."\n";
        flock(oLOG,LOCK_UN);
        close(oLOG);
        return;
      }
    }
    sleep(1);
  }
  &mylogger('can\'t open file "'.$log_file_name.'": "'.$!.'"');
  &byebye("I/O error\n");
}
