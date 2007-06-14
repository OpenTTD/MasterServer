-- phpMyAdmin SQL Dump
--
-- Database: `servers`
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
  `ip` tinytext collate utf8_unicode_ci NOT NULL,
  `port` int(11) NOT NULL default '0',
  `last_queried` datetime NOT NULL default '0000-00-00 00:00:00',
  `online` tinyint(1) NOT NULL default '0',
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
  UNIQUE KEY `ip_port` (`ip`(16),`port`),
  KEY `last_queried` (`last_queried`,`online`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

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
