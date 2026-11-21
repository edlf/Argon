// Argon: the race's crafts for the weapons' rules (app\app_race_crafts.hpp).
#include "app/app_race_crafts.hpp"
#include "app/weapons.hpp"
#include "logic/race.hpp"

int AppRaceCrafts::racerCount() const
{
    return race_.racerCount();
}

int AppRaceCrafts::slotOf(int racer) const
{
    return race_.slotOf(racer);
}

bool AppRaceCrafts::flies(int slot) const
{
    return static_cast<bool>(race_.craftInSlot(slot));
}

glm::vec3 AppRaceCrafts::position(int slot) const
{
    return race_.craftInSlot(slot)->position();
}

glm::mat4 AppRaceCrafts::frame(int slot) const
{
    return weapons_.shooterMatrix(slot);
}

bool AppRaceCrafts::targetable(int slot) const
{
    return !weapons_.craft(size_t(slot)).knockedOut();
}

std::optional<argon::weapons::WorldContact> SimWorldRay::along(const glm::vec3 &from, const glm::vec3 &to) const
{
    argon::weapons::WorldContact contact{};
    argon::TriangleSurface surface;
    if (!world_.segment(from, to, contact.point, contact.normal, surface))
    {
        return std::nullopt;
    }
    return contact;
}
