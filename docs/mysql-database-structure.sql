-- phpMyAdmin SQL Dump

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

--
-- Database: `msu`
--

-- --------------------------------------------------------

--
-- Table structure for table `newgrfs`
--

CREATE TABLE `newgrfs` (
  `grfid` int(10) unsigned NOT NULL,
  `md5sum` char(32) collate utf8_unicode_ci NOT NULL,
  `name` varchar(80) collate utf8_unicode_ci NOT NULL,
  `unknown` tinyint(1) NOT NULL,
  PRIMARY KEY  (`grfid`,`md5sum`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `servers`
--

CREATE TABLE `servers` (
  `id` int(11) NOT NULL auto_increment,
  `session_key` bigint(20) NOT NULL,
  `last_online` datetime NOT NULL default '0000-00-00 00:00:00',
  `created` datetime NOT NULL default '0000-00-00 00:00:00',
  `info_version` tinyint(3) unsigned default NULL,
  `name` tinytext collate utf8_unicode_ci,
  `revision` tinytext collate utf8_unicode_ci,
  `server_lang` tinyint(3) unsigned default NULL,
  `use_password` tinyint(1) default NULL,
  `clients_max` tinyint(3) unsigned default '0',
  `clients_on` tinyint(3) unsigned default '0',
  `companies_max` tinyint(3) unsigned default '0',
  `companies_on` tinyint(3) unsigned default '0',
  `spectators_max` tinyint(3) unsigned default '0',
  `spectators_on` tinyint(3) unsigned default '0',
  `game_date` tinytext collate utf8_unicode_ci,
  `start_date` tinytext collate utf8_unicode_ci,
  `map_name` tinytext collate utf8_unicode_ci,
  `map_width` smallint(5) unsigned default NULL,
  `map_height` smallint(5) unsigned default NULL,
  `map_set` tinyint(3) unsigned default NULL,
  `dedicated` tinyint(1) unsigned default NULL,
  `num_grfs` tinyint(3) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `session_key` (`session_key`),
  KEY `revision` (`revision`(10))
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=37881 ;

-- --------------------------------------------------------

--
-- Table structure for table `servers_ips`
--

CREATE TABLE `servers_ips` (
  `server_id` int(11) NOT NULL,
  `ipv6` tinyint(1) NOT NULL default '0',
  `ip` tinytext collate utf8_unicode_ci NOT NULL,
  `port` int(11) NOT NULL default '0',
  `last_queried` datetime NOT NULL default '0000-00-00 00:00:00',
  `last_advertised` datetime NOT NULL default '0000-00-00 00:00:00',
  `online` tinyint(1) NOT NULL default '0',
  UNIQUE KEY `ip_port` (`ip`(40),`port`),
  KEY `last_queried` (`last_queried`,`online`),
  KEY `server_id` (`server_id`),
  KEY `online` (`online`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `servers_newgrfs`
--

CREATE TABLE `servers_newgrfs` (
  `server_id` int(11) NOT NULL,
  `grfid` int(10) unsigned NOT NULL,
  `md5sum` char(32) character set latin1 NOT NULL,
  UNIQUE KEY `server_id` (`server_id`,`grfid`,`md5sum`),
  KEY `servers` (`server_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Structure for view `servers_list`
--
CREATE VIEW `servers_list` AS select `s`.`id` AS `id`,group_concat((case `i`.`online` when 1 then concat(`i`.`ip`,_utf8':',cast(`i`.`port` as char(6) charset utf8)) else NULL end) separator ', ') AS `ips`,max(`i`.`last_queried`) AS `last_queried`,max(`i`.`online`) AS `online`,`s`.`last_online` AS `last_online`,`s`.`created` AS `created`,`s`.`info_version` AS `info_version`,`s`.`name` AS `name`,`s`.`revision` AS `revision`,`s`.`server_lang` AS `server_lang`,`s`.`use_password` AS `use_password`,`s`.`clients_max` AS `clients_max`,`s`.`clients_on` AS `clients_on`,`s`.`companies_max` AS `companies_max`,`s`.`companies_on` AS `companies_on`,`s`.`spectators_max` AS `spectators_max`,`s`.`spectators_on` AS `spectators_on`,`s`.`game_date` AS `game_date`,`s`.`start_date` AS `start_date`,`s`.`map_name` AS `map_name`,`s`.`map_width` AS `map_width`,`s`.`map_height` AS `map_height`,`s`.`map_set` AS `map_set`,`s`.`dedicated` AS `dedicated`,`s`.`num_grfs` AS `num_grfs` from (`servers` `s` join `servers_ips` `i` on((`s`.`id` = `i`.`server_id`))) group by `s`.`id`;

DELIMITER $$
--
-- Procedures
--
CREATE PROCEDURE `MakeOffline`(IN p_ip TINYTEXT, IN p_port INT)
BEGIN
	UPDATE servers_ips SET online='0', last_queried='0000-00-00 00:00:00' WHERE ip=p_ip AND port=p_port;
END$$

CREATE PROCEDURE `MakeOnline`(
	IN p_ipv6 BOOL,
	IN p_ip TINYTEXT,
	IN p_port INT,
	IN p_session_key BIGINT)
BEGIN
	DECLARE v_server_id INT;
	DECLARE v_session_key BIGINT;
	SELECT server_id INTO v_server_id FROM servers_ips WHERE ip=p_ip AND port=p_port;
	IF v_server_id IS NULL OR v_server_id = 0 THEN
		INSERT INTO servers SET session_key=p_session_key, created=NOW(), last_online=NOW() ON DUPLICATE KEY UPDATE last_online=NOW();
		SELECT id INTO v_server_id FROM servers WHERE session_key=p_session_key;
	ELSE
		SELECT session_key INTO v_session_key FROM servers WHERE id=v_server_id;
		IF v_session_key <> p_session_key THEN
			UPDATE servers_ips SET server_id=0 WHERE server_id=v_server_id;
			UPDATE servers SET session_key=p_session_key WHERE id=v_server_id;
		END IF;
	END IF;
	INSERT INTO servers_ips SET server_id=v_server_id, ipv6=p_ipv6, ip=p_ip, port=p_port, last_queried='0000-00-00 00:00:00', last_advertised=NOW(), online='1' ON DUPLICATE KEY UPDATE online='1', server_id=v_server_id, last_advertised=NOW();
END$$

--
-- Functions
--
CREATE FUNCTION `UpdateGameInfo`(
	p_ip TINYTEXT,
	p_port INT,
	p_info_version INT,
	p_name TINYTEXT,
	p_revision TINYTEXT,
	p_server_lang INT,
	p_use_password INT,
	p_clients_max INT,
	p_clients_on INT,
	p_spectators_max INT,
	p_spectators_on INT,
	p_companies_max INT,
	p_companies_on INT,
	p_game_date TINYTEXT,
	p_start_date TINYTEXT,
	p_map_name TINYTEXT,
	p_map_width INT,
	p_map_height INT,
	p_map_set INT,
	p_dedicated INT,
	p_num_grfs INT) RETURNS int(11)
BEGIN
	DECLARE r_server_id INT;
	SELECT server_id INTO r_server_id FROM servers_ips WHERE ip=p_ip AND port=p_port;
	IF r_server_id IS NULL THEN
		RETURN 0;
	END IF;
	UPDATE
		servers_ips
	SET
		last_queried=NOW(),
		online='1'
	WHERE
		ip='%s' AND
		port='%d';
	UPDATE
		servers
	SET
		last_online=NOW(),
		info_version=p_info_version,
		name=p_name,
		revision=p_revision,
		server_lang=p_server_lang,
		use_password=p_use_password,
		clients_max=p_clients_max,
		clients_on=p_clients_on,
		spectators_max=p_spectators_max,
		spectators_on=p_spectators_on,
		companies_max=p_companies_max,
		companies_on=p_companies_on,
		game_date=p_game_date,
		start_date=p_start_date,
		map_name=p_map_name,
		map_width=p_map_width,
		map_height=p_map_height,
		map_set=p_map_set,
		dedicated=p_dedicated,
		num_grfs=p_num_grfs
	WHERE
		id=r_server_id;
	RETURN r_server_id;
END$$

DELIMITER ;
