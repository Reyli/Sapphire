CREATE TABLE `housepartsitem` (
	`HouseId` INT UNSIGNED NULL,
	`ItemId` INT UNSIGNED NULL,
	`CharacterId` INT UNSIGNED NULL,
	`Color` INT UNSIGNED DEFAULT '0',
	`UPDATE_DATE` DATETIME NULL
)
COLLATE='latin1_swedish_ci'
ENGINE=InnoDB;