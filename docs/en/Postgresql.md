Create skeleton database

`mkdir -p $PREFIX/var/lib/postgresql`
`initdb $PREFIX/var/lib/postgresql`

Starting the database

`pg_ctl -D $PREFIX/var/lib/postgresql start`

Similarly stop the database using

`pg_ctl -D $PREFIX/var/lib/postgresql stop`

Create User

`$ createuser --superuser --pwprompt yourUserName`

Create your database:

`$ createdb mydb`

Open your database

`$ psql mydb`

You will now see the promt

`mydb=#`