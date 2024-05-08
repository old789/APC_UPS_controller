#!/usr/local/bin/perl

use strict;
use POSIX;
use Sys::Syslog qw(:DEFAULT setlogsock);
use Fcntl ':flock';

my $log_file_name='/home/user/ups_stat/ups_stat_'.POSIX::strftime("%d_%m_%Y",localtime()).'.log';
my $toFile=1;  # 0 = syslog, 1 = file

my $name='';
my $value='';
my $pair='';
my %form=();
my $i=0;
my $progname=(($_=rindex($0,"/"))<0)?$0:substr($0,($_+1));
my $work=0;
my $msg='';

my $req_metod=exists($ENV{'REQUEST_METHOD'})?$ENV{'REQUEST_METHOD'}:"none";
my $rem_addr=exists($ENV{'REMOTE_ADDR'})?$ENV{'REMOTE_ADDR'}:"";

if ($req_metod ne 'POST') {
  &mylogger($progname, 'err', 'HTTP query with REQUEST_METHOD = '.$req_metod.($rem_addr?' from IP '.$rem_addr:''));
  &byebye("FAIL\n");
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

  if ( (!exists($form{'name'})) or ( $form{'name'} eq '' ) ) { $form{'name'}='ups'; }
  if ( (!exists($form{'model'})) or ( $form{'model'} eq '' ) ) { $form{'model'}='generic UPS'; }
  if ( (!exists($form{'uptime'})) or ( $form{'uptime'} eq '' ) ) { $form{'uptime'}='nouptime'; }

  if ( exists($form{'boot'}) ) {
    $msg='boot';
    if ( $toFile ) {
      &write2log($form{'name'}.' '.$msg);
    } else {
      &mylogger($form{'name'}, 'warning', $msg);
    }
    $work++;
  }

  if ( exists($form{'data'}) ) {
    $msg='"'.$form{'model'}.'" '.$form{'uptime'}.' data="'.$form{'data'}.'"';
    if ( $toFile ) {
      &write2log($form{'name'}.' '.$msg);
    } else {
      &mylogger($form{'name'}, 'info', $msg);
    }
    $work++;
  }

  if ( exists($form{'msg'}) ) {
    $msg='"'.$form{'model'}.'" message="'.$form{'msg'}.'"';
    if ( $toFile ) {
      &write2log($form{'name'}.' '.$msg);
    } else {
      &mylogger($form{'name'}, 'warning', $msg);
    }
    $work++;
  }

  if ( exists($form{'alarm'}) ) {
    $msg='"'.$form{'model'}.'" alert="'.$form{'alarm'}.'"';
    if ( $toFile ) {
      &write2log($form{'name'}.' '.$msg);
    } else {
      &mylogger($form{'name'}, 'emerg', $msg);
    }
    $work++;
  }

  if ( $work == 0 ) {
    &mylogger($progname, 'err', 'Incorrect message ("'.$post_query_string.'")'.($rem_addr?' from IP '.$rem_addr:''));
    &byebye("FAIL\n");
  }

&byebye("OK\n");
exit;

sub mylogger{
  my $ident=shift;
  my $priority=shift;
  my $msg=shift;
 setlogsock("unix");
 openlog($ident, "nowait", "user");
 syslog($priority, $msg);
 closelog;
 return;
}

sub byebye{
 my $outmesg=shift;

 print "Content-type: text/plain\n\n".$outmesg;
 exit;
}

sub write2log{
my $msg=shift;
my $try=0;;
my $wr_str=POSIX::strftime("%d-%m-%Y %H:%M:%S",localtime()).' '.$msg;

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
