#!/usr/local/bin/perl

use strict;
use POSIX;
use Sys::Syslog qw(:DEFAULT setlogsock);
use Fcntl ':flock';
use RRDs;

my $log_file_name='/home/user/ups_stat/ups_stat_'.POSIX::strftime("%d_%m_%Y",localtime()).'.log';
my $toFile=1;  # 0 = syslog, 1 = file
my $toRRD=0;   # 1 - write data to .rrd file, 0 - not
my $sendAlarm=0; # 1 - send, 0 - not
my $rrd_dir='/var/db/rrd/';
my $sender='send_telegram_message';

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
    if ( $toRRD ){
      &write2rrd($form{'name'}, $form{'data'});
    }
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
    if ( $sendAlarm ) {
      &send_alarm($form{'name'}.' '.$msg);
    }
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

sub write2rrd{
my $name=shift;
my $data=shift;
my $rrdfile='';

  if ( $name !~ /^[\d\w_\-\.]+$/ ) {
    &mylogger('incorrect name, RRD d\'not updated');
    return;
  }

  $rrdfile=$rrd_dir.$name.'.rrd';
  unless ( -f $rrdfile ) {
    &mylogger('file '.$rrdfile.' not found');
    return;
  }

  if ( $data !~ /^([\d\.]+)\,([\d\.]+)\,(\d+)\,([\d\.]+)\,(\d+)\,.+$/ ) {
    &mylogger('incorrect data, RRD d\'not updated');
    return;
  }
  my $ut=time();
  my $b_v=$1;
  my $t=$2;
  my $v=$3;
  my $p=$4;
  my $b_s=$5;
  RRDs::update($rrdfile,$ut.":".$v.":".$p.":".$t.":".$b_s.":".$b_v);
  my $err=RRDs::error;
  if ( $err ) {
    &mylogger('ERROR while updating '.$rrdfile.': '.$err);
  }
}

sub send_alarm{
my $msg=shift;

  unless ( -x $sender ){
    &mylogger('Program '.$sender.' not exists or non-executable');
  }
  open(TX,"|".$sender) || return;
  print TX $msg;
  close(TX);
  my $exitcode=$?>>8;
  if ($exitcode) {
    &mylogger('Program '.$sender.' finished with the exitcode "'.$exitcode.'"');
  }
}
