drop table guestbook;
vacuum;

create table guestbook(
	id integer primary key,
	name varchar(32),
	message varchar(1024),
	datetime timestamp default CURRENT_TIMESTAMP
);

