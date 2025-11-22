-- phpMyAdmin SQL Dump
-- version 5.2.1
-- https://www.phpmyadmin.net/
--
-- Anamakine: 127.0.0.1
-- Üretim Zamanı: 22 Kas 2025, 04:42:50
-- Sunucu sürümü: 10.4.32-MariaDB
-- PHP Sürümü: 8.2.12

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Veritabanı: `fire`
--

-- --------------------------------------------------------

--
-- Tablo için tablo yapısı `user_stats`
--

CREATE TABLE `user_stats` (
  `id` int(11) UNSIGNED NOT NULL,
  `user_id` int(11) UNSIGNED NOT NULL,
  `gametime` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `golds` int(11) UNSIGNED NOT NULL DEFAULT 300,
  `diamonds` int(11) UNSIGNED NOT NULL DEFAULT 30,
  `tutorial_done` tinyint(1) UNSIGNED NOT NULL DEFAULT 0,
  `characters_owned` varchar(255) NOT NULL DEFAULT '{1, 2, 3}'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Tablo döküm verisi `user_stats`
--

INSERT INTO `user_stats` (`id`, `user_id`, `gametime`, `golds`, `diamonds`, `tutorial_done`, `characters_owned`) VALUES
(1, 1, 0, 99999, 99999, 0, '{1, 2, 3}');

--
-- Dökümü yapılmış tablolar için indeksler
--

--
-- Tablo için indeksler `user_stats`
--
ALTER TABLE `user_stats`
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `user_id` (`user_id`);

--
-- Dökümü yapılmış tablolar için AUTO_INCREMENT değeri
--

--
-- Tablo için AUTO_INCREMENT değeri `user_stats`
--
ALTER TABLE `user_stats`
  MODIFY `id` int(11) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=2;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
