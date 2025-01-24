CREATE DATABASE IF NOT EXISTS `strawberrypi`;

USE `strawberrypi`;

CREATE TABLE IF NOT EXISTS `GasData` (
    `data_id` DATETIME NOT NULL,                  
    `co_ppm` FLOAT NOT NULL,                        
    `lpg_ppm` FLOAT NOT NULL,                       
    PRIMARY KEY (`data_id`)                         
);

CREATE TABLE IF NOT EXISTS `SensorData` (
    `data_id` INT(11) NOT NULL AUTO_INCREMENT,     
    `timestamp` DATETIME NOT NULL,                  
    `temperature` FLOAT NOT NULL,                  
    `humidity` FLOAT NOT NULL,                      
    `light` INT(11) NOT NULL,                       
    PRIMARY KEY (`data_id`)                         
);
