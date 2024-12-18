create table `temper` (
	`id` integer primary key autoincrement,
	`client_mac`	varchar(64) NOT NULL,
	`sensor_pin`  int(4) not null,
	`temper`	varchar(8),
       	`capture_time` timestamp NOT NULL DEFAULT (datetime('now','localtime')),
	`pool_id` int(4)
);

create table `air_pressure` (
	`id` integer primary key autoincrement,
	`client_mac`	varchar(64) NOT NULL,
	`sensor_pin`  int(4) not null,
	`air_pressure`	varchar(8),
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
	`other_param`	varchar(128) default '0',
	`description`	varchar(64) default '0'
);

create table `sensor_info_real`(
	`id` integer primary key autoincrement,
	`client_mac`	varchar(64) NOT NULL,
	`sensor_pin`  int(4) not null,
	`type` int(4),
	`pool_id`  int(4),
	`add_time` 	timestamp NOT NULL DEFAULT (datetime('now','localtime')),
	`report_interval`	int(8),
	`other_param`	varchar(128) default '0',
	`description`	varchar(64) default '0'
);

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

create table `elec` (
	`id` integer primary key autoincrement,
	`client_mac`	varchar(64) NOT NULL,
	`sensor_pin`  int(4) not null,
	`elec`	varchar(8),
       	`capture_time` timestamp NOT NULL DEFAULT (datetime('now','localtime')),
	`pool_id` int(4)
);

create table `air_temper_humidity` (
	`id` integer primary key autoincrement,
	`client_mac`	varchar(64) NOT NULL,
	`sensor_pin`  int(4) not null,
	`temper`	varchar(8),
	`humidity`	varchar(8),
       	`capture_time` timestamp NOT NULL DEFAULT (datetime('now','localtime')),
	`pool_id` int(4)
);


create table `feed_weight` (
	`id` integer primary key autoincrement,
	`client_mac`	varchar(64) NOT NULL,
	`sensor_pin`  int(4) not null,
	`feed_weight`	varchar(8),
       	`capture_time` timestamp NOT NULL DEFAULT (datetime('now','localtime')),
	`pool_id` int(4)
);

create table `feed_event` (
	`id` integer primary key autoincrement,
	`client_mac`	varchar(64) NOT NULL,
	`sensor_pin`  int(4) not null,
	`feed_weight`	varchar(8),
       	`capture_time` timestamp NOT NULL DEFAULT (datetime('now','localtime')),
	`pool_id` int(4)
);

create table `check_list` (
	`id` integer primary key autoincrement,
	`client_mac`	varchar(64) NOT NULL,
	`sensor_pin`  int(4) not null,
	`type` int(4),
    `off_time` int(8),
	`low_value` int(8),
	`hight_value` int(8),
	`is_alerting` int(4) default 0,
	`alert_times` int(6) default 0,
	`enable` int(4)
);

create table `pool` (
	`id` integer primary key autoincrement,
	`area`	varchar(8),
	`deep`  	varchar(8),
	`shrimp_num`	integer DEFAULT 0,
       	`start_feed_time` timestamp NOT NULL DEFAULT (datetime('now','localtime'))
);

create unique index sensor_index on sensor_info(client_mac, sensor_pin);
