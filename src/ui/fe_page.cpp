// Argon: a front-end page's UI sounds (ui\fe_page.hpp, ui\fe_sounds.hpp)
#include "ui/fe_page.hpp"

#include "audio/audio.hpp"

void FePage::play(menu::UiEvent event) const
{
    const std::optional<menu::UiSound> sound = sounds().soundFor(event);
    if (sound)
    {
        audio.playUi(int(*sound));
    }
}
