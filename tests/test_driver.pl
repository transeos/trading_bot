#!/usr/bin/perl -w

#-*- Perl -*-
#
# *****************************************************************
#
# WARRANTY:
# Use all material in this file at your own risk.
#
# Created by Hiranmoy Basak on 12/04/18.
#

use strict;
use warnings;
use Cwd;
use Time::HiRes;
use Term::ANSIColor;
use File::Path;

my $gTraderHome = $ENV{TRADER_HOME};
my $gNumInstances = 1;
my $gTypes = "";
my @gTests = ();
my $gTestDir = getcwd()."/crypto_tests";
my $retryMode = 0;
my $buildDir = "build";

sub PrintHelp() {
  #print "n=<n> (optional): n number of parallel instances\n";
  print "t=<test type1><test type2>... (optional) : specify test type(s)\n";
  print "build=<build directory name> (optional) (default: build) (example : build=cmake_build_debug\n"
}

sub ProcessArguments() {
  for (my $argIdx = 0; $argIdx <= $#ARGV; $argIdx++) {
    my $curArg = $ARGV[$argIdx];

    if (($curArg eq "-h") or ($curArg eq "--help")) {
      PrintHelp();
      exit(0);
    }

    my @words = split /=/, $curArg;

    if ($words[0] eq "n") {
      print color('red')."Error: Not yet supported : ".$curArg."\n".color('reset');
      exit(1);

      $gNumInstances = $words[1];
      print "Number of parallel instances = ".$gNumInstances."\n";
    }
    elsif($words[0] eq "t") {
      $gTypes = $words[1];
      print "Test type(s) = ".$gTypes."\n";
    }
    elsif($words[0] eq "-r") {
      $retryMode = 1;
      print "Retry mode\n";
    }
    elsif($words[0] eq "build") {
      $buildDir = $words[1];
      print " = build directory name = ".$buildDir."\n";
    }
    else {
      print color('red')."Error: Invalid argument : ".$curArg."\n".color('reset');
      exit(1);
    }
  }
  print "\n";
}

sub CheckIfValidTraderHome() {
  print "TRADER_HOME = ".$gTraderHome."\n\n";

  if (! -d $gTraderHome) {
    print "Error: Invalid TRADER_HOME\n";
    exit(1);
  }
}

sub ltrim { my $s = shift; $s =~ s/^\s+//;       return $s };
sub rtrim { my $s = shift; $s =~ s/\s+$//;       return $s };
sub  trim { my $s = shift; $s =~ s/^\s+|\s+$//g; return $s };

sub getTestLists() {
  my @tests = `$gTraderHome/$buildDir/test_cryptotrader -l $gTypes`;

  for (my $lineIdx = 1; $lineIdx <= $#tests - 3; $lineIdx = $lineIdx + 2) {
    my $testName = $tests[$lineIdx];
    chomp $testName;
    $testName = trim($testName);

    push(@gTests, $testName);
  }
}

sub WriteScripts() {
  for (my $testIdx = 0; $testIdx <= $#gTests; $testIdx++) {
    my $testName = $gTests[$testIdx];

    #start from a clean directory
    mkdir "$gTestDir/$testName";

    my $script = "$gTestDir/$testName/BotTest.sh";
    open(my $fp, '>', $script) or die "Could not open file '$script' $!";

    print $fp "#!/bin/bash\n";
    print $fp "\n";
    print $fp "RED=`tput setaf 1`\n";
    print $fp "GREEN=`tput setaf 2`\n";
    print $fp "BLUE=`tput setaf 4`\n";
    print $fp "NC=`tput sgr0`\n";
    print $fp "\n";
    print $fp "echo === \${BLUE}$testName started \${NC}=== | tee BotTest.log\n";
    print $fp "\n";
    print $fp "echo '' >> BotTest.log\n";
    print $fp "\n";
    print $fp "$gTraderHome/$buildDir/test_cryptotrader $testName >> BotTest.log\n";
    print $fp "returnCode=\$?\n";
    print $fp "\n";
    print $fp "if [ \$returnCode = 0 ]\n";
    print $fp "then\n";
    print $fp "  returnCode=`grep -c FAILED BotTest.log`\n";
    print $fp "fi\n";
    print $fp "\n";
    print $fp "echo '' >> BotTest.log\n";
    print $fp "\n";
    print $fp "if [ \$returnCode = 0 ]\n";
    print $fp "then\n";
    print $fp "  echo === \${GREEN}$testName passed \${NC}=== | tee -a BotTest.log\n";
    print $fp "  touch .passed\n";
    print $fp "else\n";
    print $fp "  echo === \${RED}$testName failed \${NC}=== | tee -a BotTest.log\n";
    print $fp "  touch .failed\n";
    print $fp "  exit 1\n";
    print $fp "fi\n";

    close $fp;

    system("chmod +x $script");
  }
}

sub main {
  ProcessArguments();

  CheckIfValidTraderHome();

  getTestLists();

  my @currentTests = ();

  if ($retryMode) {
    my % failed = ();

    for (my $testIdx = 0; $testIdx <= $#gTests; $testIdx++) {
      my $testName = $gTests[$testIdx];

      if (-f "$gTestDir/$testName/.failed") {
        $failed{$testName} = 1;
      }
    }

    #launch failed testcases
    foreach
      my $test(keys % failed) {
        system("cd $gTestDir/$test; rm -f .failed; sleep 4; ./BotTest.sh &");
      }

    @currentTests = keys % failed;
  }
  else {
    print "Cleaning current test directory (".$gTestDir.")\n";
    system("rm -rf $gTestDir; \\mkdir $gTestDir");

    #write bash scripts
    WriteScripts();

    #launch testcases
    for (my $testIdx = 0; $testIdx <= $#gTests; $testIdx++) {
      my $testName = $gTests[$testIdx];
      system("cd $gTestDir/$testName; sleep 5; ./BotTest.sh &");
    }

    @currentTests = @gTests;
  }

  #check whether all testcases are finished
  my $totalTests = ($#currentTests + 1);
  my % status = ();
  my $prevRunning = $totalTests;
  while (1) {
    for (my $testIdx = 0; $testIdx <= $#currentTests; $testIdx++) {
      my $testName = $currentTests[$testIdx];

      if (exists($status{$testName})) {
        next;
      }

      if (-f "$gTestDir/$testName/.passed") {
        $status{$testName} = 1;
      }
      elsif(-f "$gTestDir/$testName/.failed") {
        $status{$testName} = 0;
      }
    }

    my $running = ($totalTests - (scalar keys % status));
    if ($running eq $prevRunning) {
      next;
    }
    $prevRunning = $running;

    my $failure = 0;
    foreach
      my $test(keys % status) {
        my $passed = $status{$test};

        if ($passed eq 0) {
          print $test.", ";
          $failure = 1;
        }
      }
    if ($failure ne 0) {
      print color('red')."failed, ".color('reset');
    }

    if ($running eq 0) {
      print color('blue')."\nAll tests finished !\n".color('reset');
      last;
    }

    print $running." tests running\n";

    #sleep for 10 sec
    sleep(10);
  }
}

main;
