MariaDB is a community-developed, commercially supported fork of the
MySQL relational database management system.

## Installing package

Install package "mariadb":

`pkg install mariadb`

The installation should finish without any error. If error occurred, you
probably need to report the issue on
[Github](https://github.com/termux/termux-packages/issues).

The command shown above will also initialize the database with **2
all-privilege accounts** (introduced perhaps in MariaDB 10.4.x). The
first one is "root" which is inaccessible and the second one with name
of your Termux user (check with command `id -un` or `whoami`).

To enable access to root account, you need to login with your Termux
user name

`mysql -u $(whoami)`

and manually change password for root

`use mysql;`
`set password for 'root'@'localhost' = password('YOUR_ROOT_PASSWORD_HERE');`
`flush privileges;`
`quit;`

Verify that you are able to login as 'root' with `mysql -u root -p`. You
will need to provide password set in previous step.

## Using package

Whenever you want to access MySQL database manually through command line
or with some program (web application), you need to start MySQL server:

`mysqld_safe`

Then you should be able to connect to database, e.g. with
`mysql -u root -p`.