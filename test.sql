create table `temper` (
	`id` integer primary key autoincrement,
	`client_mac`	varchar(64) NOT NULL,
	`sensor_pin`  int(4) not null,
	`temper`	varchar(8),
       	`capture_time` timestamp NOT NULL DEFAULT (datetime('now','localtime')),
	`pool_id` int(4)
);

create table `sensor_info`(
	`id` integer primary key autoincrement,
	`client_mac`	varchar(64) NOT NULL,
	`sensor_pin`  int(4) not null,
	`type` int(4),
	`pool_id`  int(4),
	`add_time` 	timestamp NOT NULL DEFAULT (datetime('now','localtime')),
	`report_interval`	int(8),
	`other_param`	varchar(32),
	`description`	varchar(64)
)

create table `ph` (
	`id` integer primary key autoincrement,
	`client_mac`	varchar(64) NOT NULL,
	`sensor_pin`  int(4) not null,
	`ph`	varchar(8),
       	`capture_time` timestamp NOT NULL DEFAULT (datetime('now','localtime')),
	`pool_id` int(4)
);

create table `water_level` (
	`id` integer primary key autoincrement,
	`client_mac`	varchar(64) NOT NULL,
	`sensor_pin`  int(4) not null,
	`water_level`	varchar(8),
       	`capture_time` timestamp NOT NULL DEFAULT (datetime('now','localtime')),
	`pool_id` int(4)
);

create table `feed` (
	`id` integer primary key autoincrement,
	`client_mac`	varchar(64) NOT NULL,
	`sensor_pin`  int(4) not null,
	`feed_weight`	varchar(8),
       	`capture_time` timestamp NOT NULL DEFAULT (datetime('now','localtime')),
	`pool_id` int(4)
);

create unique index sensor_index on sensor_info(client_mac, sensor_pin);