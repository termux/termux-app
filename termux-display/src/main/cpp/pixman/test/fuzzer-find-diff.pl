#!/usr/bin/env perl

$usage = "Usage:
  fuzzer-find-diff.pl reference_binary new_binary [number_of_tests_to_run]

The first two input arguments are the commands to run the test programs
based on fuzzer_test_main() function from 'util.c' (preferably they should
be statically compiled, this can be achieved via '--disable-shared' pixman
configure option). The third optional argument is the number of test rounds
to run (if not specified, then testing runs infinitely or until some problem
is detected).

Usage examples:
  fuzzer-find-diff.pl ./blitters-test-with-sse-disabled ./blitters-test 9000000
  fuzzer-find-diff.pl ./blitters-test \"ssh ppc64_host /path/to/blitters-test\"
";

$#ARGV >= 1 or die $usage;

$batch_size = 10000;

if ($#ARGV >= 2) {
    $number_of_tests = int($ARGV[2]);
} else {
    $number_of_tests = -1
}

sub test_range {
    my $min = shift;
    my $max = shift;

    # check that [$min, $max] range is "bad", otherwise return
    if (`$ARGV[0] $min $max 2>/dev/null` eq `$ARGV[1] $min $max 2>/dev/null`) {
        return;
    }

    # check that $min itself is "good", otherwise return
    if (`$ARGV[0] $min 2>/dev/null` ne `$ARGV[1] $min 2>/dev/null`) {
        return $min;
    }

    # start bisecting
    while ($max != $min + 1) {
        my $avg = int(($min + $max) / 2);
        my $res1 = `$ARGV[0] $min $avg 2>/dev/null`;
        my $res2 = `$ARGV[1] $min $avg 2>/dev/null`;
        if ($res1 ne $res2) {
            $max = $avg;
        } else {
            $min = $avg;
        }
    }
    return $max;
}

$base = 1;
while ($number_of_tests <= 0 || $base <= $number_of_tests) {
    printf("testing %-12d\r", $base + $batch_size - 1);
    my $res = test_range($base, $base + $batch_size - 1);
    if ($res) {
        printf("Failure: results are different for test %d:\n", $res);

        printf("\n-- ref --\n");
        print `$ARGV[0] $res`;
        printf("-- new --\n");
        print `$ARGV[1] $res`;

        printf("The problematic conditions can be reproduced by running:\n");
        printf("$ARGV[1] %d\n", $res);

        exit(1);
    }
    $base += $batch_size;
}
printf("Success: %d tests finished\n", $base - 1);
