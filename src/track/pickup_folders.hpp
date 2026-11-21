// Argon: the data\objects folder of a pickup's model, from its track.dat type name
#pragma once
#include <string>

std::string pickupFolder(const std::string &type);         // POWERUPRED -> powerup_red
std::string fruityPickupFolder(const std::string &type);   // POWERUPRED_NS -> powerup_red_ee_ns (Fruity Fruit)
