// Argon: the HUD's warnings about the player's craft (logic\race_warnings.hpp): RESET and WRONG WAY
// (craft_race_info 0x5ce70, each 60 Hz step), "Shield Low" (the power-up panel's draw 0x4eb20)
#include "app/app.hpp"
#include "logic/music_cues.hpp"

void App::resetRaceWarnings()
{
    raceWarnings.reset();
    shieldLow.reset();
    session.warningRespawnsSeen = sim.respawns();
    leadWatch.reset();
}

// the route: from the craft's AI node to the next one (race slot +0 at 0x5972f0, 0xebc11); Shield Low is posted only
// while the race runs (0x4eb20: the player not finished, 0x52af9f and the post-race state clear)
void App::stepRaceWarnings()
{
    argon::RaceWarnings::Step step;
    step.respawned = sim.respawns() != session.warningRespawnsSeen;
    session.warningRespawnsSeen = sim.respawns();
    step.alive = weapons.craft(0).energy() > 0.f;
    step.forward = glm::vec3(sim.matrix()[2]);
    if (aiGraph.nodes.size() > 2)
    {
        const int node = std::clamp(sim.node(), 0, int(aiGraph.nodes.size()) - 1);
        step.route = aiGraph.nodes[size_t(aiGraph.next(node))].pos - aiGraph.nodes[size_t(node)].pos;
    }
    raceWarnings.step(step, 1.f / 60.f);
    if (!logic().hasWeapons())
    {
        return;
    }
    shieldLow.step(weapons.shieldPercent(0) * 0.01f, 1.f / 60.f);
    if (shieldLow.popupDue() && session.raceEnd == kRacing)
    {
        awardPoints(0, 13);
        audio.raceSfx().playOnListener(argon::GenericSound::LowEnergy);   // 0x4ed72: sound 0x23
    }
    if (shieldLow.wentLow())
    {
        audio.raiseRaceMusic();
    }
    if (shieldLow.recovered())
    {
        audio.lowerRaceMusic();
    }
}

// 0x63140: the player's lap count going up changes the music's mode on the final lap and lap 2
void App::cueLapMusic()
{
    const int lap = sim.laps() + 1;
    const int finalLap = logic().hasLapLimit() ? session.hudLaps : 0;
    if (argon::music::finalLapStarts(lap, finalLap))
    {
        audio.raiseRaceMusic();
    }
    if (argon::music::secondLapStarts(lap, finalLap))
    {
        audio.playSecondLapMusic();
    }
}

// 0x635c0: the player taking or losing the lead
void App::cueLeadMusic()
{
    const std::vector<int> places = race.places();
    if (places.empty())
    {
        return;
    }
    const argon::music::LeadCue cue = leadWatch.see(places[0]);
    if (cue == argon::music::LeadCue::TookTheLead)
    {
        audio.raiseRaceMusic();
    }
    if (cue == argon::music::LeadCue::LostTheLead)
    {
        audio.lowerRaceMusic();
    }
}
