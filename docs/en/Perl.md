Perl is a high-level, general-purpose, interpreted, dynamic programming
language. It is used for system administration, network programming,
finance, bioinformatics, and other applications. It has been nicknamed
"the Swiss Army chainsaw of scripting languages" because of its
flexibility and power, and also its ugliness.

In Termux Perl can be installed by executing this command

`pkg install perl`

Note that Termux provides only Perl 5 currently.

## Package management

Perl uses `cpan` utility for module management. However we recommend to
install `cpanminus` for installing modules easier:

`cpan App::cpanminus`

Now you can install Perl modules using

`cpanm Module::Name`

It is highly recommended to have `build-essential` Termux package to be
installed `pkg install build-essential`, so Perl will be able to build
native extensions.