create table `temper` (
	`id` integer primary key autoincrement,
	`client_mac`	varchar(64) NOT NULL,
	`client_temper_index` int not null,
	`temper`	varchar(8),
       	`capture_time` timestamp NOT NULL DEFAULT (datetime('now','localtime'))
);	
