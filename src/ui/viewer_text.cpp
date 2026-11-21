// Argon: the viewer's own menu text in each language (viewer_text.hpp).
#include "ui/viewer_text.hpp"

#include <array>

std::u16string ViewerText::in(Language language) const
{
    const char16_t *chosen = english;
    switch (language)
    {
    case Language::French:
        chosen = french;
        break;
    case Language::German:
        chosen = german;
        break;
    case Language::Italian:
        chosen = italian;
        break;
    case Language::Japanese:
        chosen = japanese;
        break;
    default:
        break;
    }
    return chosen ? chosen : english;
}

namespace viewer_text
{

const RowText kSave{
    {u"Save", u"Sauvegarder", u"Speichern", u"Salva", u"セーブ"},
    {u"Keeps these settings and returns to Argon Options; B returns without them",
     u"Conserve ces paramètres et retourne aux options Argon. B retourne sans les conserver.",
     u"Diese Einstellungen behalten und zu den Argon-Optionen zurückkehren; B kehrt ohne sie zurück",
     u"Mantieni queste impostazioni e torna alle opzioni Argon. B torna indietro senza salvarle.",
     u"設定をセーブして Argon オプションにもどる。B でセーブせずにもどる"}};

const RowText kGeneral{
    {u"General", u"Général", u"Allgemein", u"Generale", u"ジェネラル"},
    {u"Language, intro videos and the FPS meter", u"Langue, vidéos d'introduction et compteur FPS.", u"Sprache, Introvideos und FPS-Anzeige",
     u"Lingua, video introduttivi e contatore FPS.", u"言語、イントロムービー、FPS メーター"}};

const RowText kGraphics{
    {u"Graphics", u"Graphismes", u"Grafik", u"Grafica", u"グラフィック"},
    {u"Renderer, resolution, window, anti-aliasing and the HUD's size", u"Rendu, résolution, fenêtre, anticrénelage et taille du VTH.",
     u"Renderer, Auflösung, Fenster, Kantenglättung und HUD-Größe", u"Renderer, risoluzione, finestra, antialiasing e dimensioni dell'HUD.",
     u"レンダラー、解像度、ウィンドウ、アンチエイリアス、HUD サイズ"}};

const RowText kAudio{
    {u"Audio", u"Audio", u"Audio", u"Audio", u"オーディオ"},
    {u"Speakers and how the 3D sounds are heard", u"Haut-parleurs et rendu des sons 3D.", u"Lautsprecher und wie die 3D-Klänge zu hören sind",
     u"Altoparlanti e resa dei suoni 3D.", u"スピーカーと 3D サウンドの聞こえ方"}};

const RowText kGameFixes{
    {u"Game Fixes", u"Correctifs du jeu", u"Spielkorrekturen", u"Correzioni del gioco", u"ゲームフィックス"},
    {u"Fixes for the game's own glitches", u"Corrige les défauts du jeu d'origine.", u"Korrigiert Fehler des Originalspiels",
     u"Corregge i difetti del gioco originale.", u"ゲームのバグをなおす"}};

const RowText kKeyBindings{
    {u"Key Bindings", u"Touches", u"Tastenbelegung", u"Tasti", u"キー設定"},
    {u"Choose the keyboard's keys for the craft", u"Choisit les touches du clavier pour le vaisseau.", u"Tastaturtasten für das Fahrzeug wählen",
     u"Scegli i tasti della tastiera per il veicolo.", u"マシンのキーボードのキーをえらぶ"}};

const ViewerText kKeyActions[kKeyActionCount] = {
    {u"Throttle", u"Accélérer", u"Gas", u"Accelera", u"アクセル"},
    {u"Brake", u"Freiner", u"Bremse", u"Freno", u"ブレーキ"},
    {u"Steer Left", u"Gauche", u"Links lenken", u"Sterza a sinistra", u"左に曲がる"},
    {u"Steer Right", u"Droite", u"Rechts lenken", u"Sterza a destra", u"右に曲がる"},
    {u"Left Airbrake", u"Aérofrein gauche", u"Linke Luftbremse", u"Aerofreno sinistro", u"左エアブレーキ"},
    {u"Right Airbrake", u"Aérofrein droit", u"Rechte Luftbremse", u"Aerofreno destro", u"右エアブレーキ"},
    {u"Dive", u"Piquer", u"Abtauchen", u"Picchiata", u"ダイブ"},
    {u"Climb", u"Monter", u"Steigen", u"Cabrata", u"クライム"},
    {u"Turbo", u"Turbo", u"Turbo", u"Turbo", u"ターボ"},
    {u"Reset", u"Replacer", u"Zurücksetzen", u"Riposiziona", u"リセット"},
    {u"Fire Red", u"Tir rouge", u"Rot feuern", u"Fuoco rosso", u"赤のウェポン"},
    {u"Fire Blue", u"Tir bleu", u"Blau feuern", u"Fuoco blu", u"青のウェポン"},
    {u"Fire Yellow", u"Tir jaune", u"Gelb feuern", u"Fuoco giallo", u"黄のウェポン"},
    {u"Change View", u"Changer de vue", u"Ansicht wechseln", u"Cambia visuale", u"カメラを変える"},
};

const ViewerText kKeyBindingTicker{u"A changes the key, X clears it; left / right picks the first or second key",
                                   u"A change la touche, X l'efface. Gauche / droite choisit la première ou la deuxième touche.",
                                   u"A ändert die Taste, X löscht sie; links / rechts wählt die erste oder zweite Taste",
                                   u"A cambia il tasto, X lo cancella. Sinistra / destra sceglie il primo o il secondo tasto.",
                                   u"A でキーを変える、X で消す。左右で 1 番目か 2 番目のキーをえらぶ"};
const ViewerText kResetPageQuestion{u"Are you sure you want to reset this page's settings to their defaults?",
                                    u"Voulez-vous vraiment rétablir les paramètres par défaut de cette page ?",
                                    u"Sollen die Einstellungen dieser Seite wirklich auf die Standardwerte zurückgesetzt werden?",
                                    u"Ripristinare davvero le impostazioni predefinite di questa pagina?",
                                    u"このページの設定をデフォルトにもどしますか?"};
const ViewerText kPressAKey{u"Press the new key; Esc keeps the old one", u"Appuyez sur la nouvelle touche. Échap garde l'ancienne.",
                            u"Neue Taste drücken; Esc behält die alte", u"Premi il nuovo tasto. Esc mantiene quello vecchio.",
                            u"新しいキーをおしてください。Esc で前のキーのまま"};

const RowText kLanguage{
    {u"Language", u"Langue", u"Sprache", u"Lingua", u"言語"},
    {u"The language of the menus' text, the subtitles and the intro video",
     u"Langue des textes des menus, des sous-titres et de la vidéo d'introduction.",
     u"Sprache der Menütexte, der Untertitel und des Introvideos",
     u"Lingua dei testi dei menu, dei sottotitoli e del video introduttivo.",
     u"メニュー、字幕、イントロムービーの言語"}};

const RowText kRenderer{
    {u"Renderer", u"Rendu", u"Renderer", u"Renderer", u"レンダラー"},
    {u"Vulkan or Direct3D 12; needs a restart: takes effect at the next start",
     u"Vulkan ou Direct3D 12. Nécessite un redémarrage : pris en compte au prochain lancement.",
     u"Vulkan oder Direct3D 12; erfordert einen Neustart, wirkt ab dem nächsten Start",
     u"Vulkan o Direct3D 12. Richiede il riavvio: attivo dal prossimo avvio.",
     u"Vulkan または Direct3D 12。再起動が必要: 次回の起動から有効"}};

const RowText kWidescreen{
    {u"Widescreen", u"Écran large", u"Breitbild", u"Schermo panoramico", u"ワイドスクリーン"},
    {u"Off keeps the 4:3 layout of the menus and the HUD in a wide window",
     u"Désactivé : les menus et le VTH gardent la disposition 4:3 dans une fenêtre large.",
     u"Aus behält das 4:3-Layout von Menüs und HUD in einem breiten Fenster bei",
     u"Disattivato mantiene il formato 4:3 di menu e HUD in una finestra larga.",
     u"Off: ワイドなウィンドウでもメニューと HUD を 4:3 で表示"}};

const RowText kHudSize{
    {u"HUD Size", u"Taille du VTH", u"HUD-Größe", u"Dimensioni HUD", u"HUD サイズ"},
    {u"The size of the race HUD", u"Taille du VTH pendant la course.", u"Größe des Renn-HUDs", u"Dimensioni dell'HUD in gara.",
     u"レース中の HUD の大きさ"}};

const RowText kResolution{
    {u"Resolution", u"Résolution", u"Auflösung", u"Risoluzione", u"解像度"},
    {u"The picture's size: the monitor's mode in fullscreen, the window's size out of it",
     u"Taille de l'image : mode de l'écran en plein écran, taille de la fenêtre sinon.",
     u"Bildgröße: im Vollbild der Modus des Monitors, sonst die Fenstergröße",
     u"Dimensioni dell'immagine: modalità del monitor a schermo intero, altrimenti dimensioni della finestra.",
     u"画面の大きさ: フルスクリーンではモニターのモード、それ以外ではウィンドウの大きさ"}};

const RowText kFullscreen{
    {u"Fullscreen", u"Plein écran", u"Vollbild", u"Schermo intero", u"フルスクリーン"},
    {u"On switches the monitor to the resolution and fills it (F11)",
     u"Activé : l'écran passe à la résolution choisie et l'image le remplit (F11).",
     u"Ein schaltet den Monitor auf die Auflösung und füllt ihn aus (F11)",
     u"Attivato imposta il monitor sulla risoluzione e lo riempie (F11).",
     u"On: モニターを解像度に合わせて全画面で表示 (F11)"}};

const RowText kVsync{
    {u"VSync", u"Synchro verticale", u"VSync", u"VSync", u"VSync"},
    {u"On shows each frame on the monitor's refresh; Off draws as fast as it can, and may tear",
     u"Activé : chaque image suit le rafraîchissement de l'écran. Désactivé : affichage au plus vite, avec possible déchirement.",
     u"Ein zeigt jedes Bild mit der Bildwiederholrate des Monitors; Aus zeichnet so schnell wie möglich, mit möglichem Tearing",
     u"Attivato mostra ogni fotogramma al refresh del monitor; disattivato disegna il più velocemente possibile, con possibile tearing.",
     u"On: モニターのリフレッシュに合わせて表示。Off: できるだけ速く表示 (画面がずれることがあります)"}};

const RowText kMsaa{
    {u"MSAA", u"MSAA", u"MSAA", u"MSAA", u"MSAA"},
    {u"Multisampling smooths every edge, the menus' models too; takes effect at the next start",
     u"Le multi-échantillonnage lisse tous les contours, y compris les modèles des menus. Pris en compte au prochain lancement.",
     u"Multisampling glättet alle Kanten, auch die Modelle der Menüs; wirkt ab dem nächsten Start",
     u"Il multisampling smussa tutti i bordi, anche i modelli dei menu. Attivo dal prossimo avvio.",
     u"マルチサンプリングですべてのエッジをなめらかに (メニューのモデルも)。次回の起動から有効"}};

const RowText kPostAa{
    {u"Post AA", u"Post AA", u"Post-AA", u"Post AA", u"ポスト AA"},
    {u"Smooths the 3D view's edges after it is drawn: FXAA is softer, SMAA sharper; the HUD stays sharp",
     u"Lisse les contours de la vue 3D après son rendu : FXAA est plus doux, SMAA plus net. Le VTH reste net.",
     u"Glättet die Kanten der 3D-Ansicht nach dem Zeichnen: FXAA ist weicher, SMAA schärfer; das HUD bleibt scharf",
     u"Smussa i bordi della vista 3D dopo il disegno: FXAA è più morbido, SMAA più nitido. L'HUD resta nitido.",
     u"3D 画面のエッジを後からなめらかに: FXAA はやわらかく、SMAA はシャープ。HUD はそのまま"}};

const RowText kTranslucency{
    {u"Translucency", u"Transparence", u"Transparenz", u"Trasparenza", u"トランスペアレンシー"},
    {u"Game Order blends see-through surfaces as the game does, glitches included; Order Independent shows what is behind glass whatever the order",
     u"Ordre du jeu mélange les surfaces transparentes comme le jeu, défauts compris. Indépendant de l'ordre montre ce qui est derrière les vitres "
     u"quel que soit l'ordre.",
     u"Spielreihenfolge mischt durchsichtige Flächen wie das Spiel, Fehler inklusive; Reihenfolgeunabhängig zeigt, was hinter Glas ist, egal in "
     u"welcher Reihenfolge",
     u"Ordine del gioco fonde le superfici trasparenti come il gioco, difetti compresi; Indipendente dall'ordine mostra ciò che sta dietro il vetro "
     u"in qualsiasi ordine.",
     u"ゲームの順番: すけて見える面をゲームと同じ順番で表示 (ゲームのバグも同じ)。順番に関係なし: ガラスのうしろを順番に関係なく表示"}};

const RowText kSpeedBlur{
    {u"Speed Blur", u"Flou de vitesse", u"Geschwindigkeitsunschärfe", u"Sfocatura di velocità", u"スピードブラー"},
    {u"Off removes the blur the race lays over the picture above 720 km/h",
     u"Désactivé : supprime le flou que la course applique à l'image au-delà de 720 km/h.",
     u"Aus entfernt die Unschärfe, die das Rennen über 720 km/h über das Bild legt",
     u"Disattivato rimuove la sfocatura che la gara applica all'immagine oltre i 720 km/h.",
     u"オフ: 720 km/h をこえたときに画面にかかるブラーをなくします"}};

const RowText kMarsBlurFix{
    {u"Mars Flicker Fix", u"Correctif du scintillement sur Mars", u"Mars-Flackerkorrektur", u"Correzione sfarfallio su Marte",
     u"火星のブラーのちらつきをなおす"},
    {u"On the Mars tracks, fades the speed blur without zooming it, so the gantries over the track no longer flicker",
     u"Sur les circuits de Mars, le flou de vitesse est un simple fondu sans zoom : les portiques au-dessus de la piste ne scintillent plus.",
     u"Auf den Mars-Strecken blendet es die Unschärfe ohne Zoom ein, die Brücken über der Strecke flackern nicht mehr",
     u"Sui circuiti di Marte la sfocatura è una dissolvenza senza zoom: i portali sopra la pista non sfarfallano più.",
     u"火星のコースでブラーをズームなしで重ねるので、コースの上のゲートがちらつきません"}};

const RowText kFmvs{
    {u"FMVs", u"Cinématiques", u"Videos", u"Filmati", u"ムービー"},
    {u"Off skips the intro videos at start-up and opens on the title screen",
     u"Désactivé : les vidéos d'introduction sont passées au démarrage et le jeu s'ouvre sur l'écran titre.",
     u"Aus überspringt die Introvideos beim Start und öffnet den Titelbildschirm",
     u"Disattivato salta i video introduttivi all'avvio e apre la schermata del titolo.",
     u"Off: 起動時のイントロムービーをスキップしてタイトル画面から始める"}};

const RowText kFpsMeter{
    {u"FPS Meter", u"Compteur FPS", u"FPS-Anzeige", u"Contatore FPS", u"FPS メーター"},
    {u"Shows the frame rate, the frame times and the memory in use, top right (F2)",
     u"Affiche en haut à droite la fréquence d'images, les temps d'image et la mémoire utilisée (F2).",
     u"Zeigt oben rechts Bildrate, Bildzeiten und belegten Speicher (F2)",
     u"Mostra in alto a destra frequenza dei fotogrammi, tempi dei fotogrammi e memoria in uso (F2).",
     u"右上にフレームレート、フレーム時間、使用中のメモリを表示 (F2)"}};

const RowText kSpeakers{
    {u"Speakers", u"Haut-parleurs", u"Lautsprecher", u"Altoparlanti", u"スピーカー"},
    {u"Stereo, or 5.1 as the Xbox plays in Dolby Digital; takes effect at the next start",
     u"Stéréo, ou 5.1 comme la Xbox en Dolby Digital. Pris en compte au prochain lancement.",
     u"Stereo oder 5.1 wie die Xbox in Dolby Digital; wirkt ab dem nächsten Start",
     u"Stereo, o 5.1 come la Xbox in Dolby Digital. Attivo dal prossimo avvio.",
     u"ステレオ、または Xbox の Dolby Digital と同じ 5.1。次回の起動から有効"}};

const RowText kHrtf{
    {u"HRTF", u"HRTF", u"HRTF", u"HRTF", u"HRTF"},
    {u"How the 3D sounds reach each ear: the Xbox's own filters, or MIT's KEMAR dummy head (needs hrtf_kemar.bin beside game_data)",
     u"Comment les sons 3D arrivent à chaque oreille : les filtres de la Xbox, ou la tête artificielle KEMAR du MIT (nécessite hrtf_kemar.bin "
     u"à côté de game_data).",
     u"Wie die 3D-Klänge jedes Ohr erreichen: die Filter der Xbox oder der KEMAR-Kunstkopf des MIT (braucht hrtf_kemar.bin neben game_data)",
     u"Come i suoni 3D arrivano a ciascun orecchio: i filtri della Xbox o la testa artificiale KEMAR del MIT (richiede hrtf_kemar.bin accanto "
     u"a game_data).",
     u"3D サウンドの聞こえ方: Xbox のフィルター、または MIT の KEMAR ダミーヘッド (game_data のとなりに hrtf_kemar.bin が必要)"}};

const RowText kKemarTaps{
    {u"KEMAR Taps", u"Coefficients KEMAR", u"KEMAR-Taps", u"Tap KEMAR", u"KEMAR タップ数"},
    {u"MIT KEMAR's filter length: longer keeps more of the dummy head's sound, at more CPU; only with HRTF on MIT KEMAR",
     u"Longueur du filtre MIT KEMAR : plus long garde plus du son de la tête artificielle, mais utilise plus le processeur. Seulement avec HRTF "
     u"sur MIT KEMAR.",
     u"Filterlänge von MIT KEMAR: länger bewahrt mehr vom Klang des Kunstkopfs, bei mehr CPU-Last; nur mit HRTF auf MIT KEMAR",
     u"Lunghezza del filtro MIT KEMAR: più lungo conserva più suono della testa artificiale, con più carico sulla CPU. Solo con HRTF su MIT KEMAR.",
     u"MIT KEMAR のフィルターの長さ: 長いほどダミーヘッドの音に近いが、CPU を多く使う。HRTF が MIT KEMAR のときのみ"}};

const ViewerText kGameOrder{u"Game Order", u"Ordre du jeu", u"Spielreihenfolge", u"Ordine del gioco", u"ゲームの順番"};
const ViewerText kOrderIndependent{u"Order Independent", u"Indépendant de l'ordre", u"Reihenfolgeunabhängig", u"Indipendente dall'ordine",
                                   u"順番に関係なし"};
const ViewerText kStereo{u"Stereo", u"Stéréo", u"Stereo", u"Stereo", u"ステレオ"};

const ViewerText kArgonOptions{u"Argon Options", u"Options Argon", u"Argon-Optionen", u"Opzioni Argon", u"Argon オプション"};
const ViewerText kArgonOptionsTicker{u"Adjust Argon's Options", u"Règle les options d'Argon.", u"Argon-Optionen anpassen", u"Regola le opzioni di Argon.",
                                     u"Argon のオプションの変更"};
const ViewerText kRendererRestart{u"Argon needs a restart to switch to the new renderer.", u"Argon doit redémarrer pour passer au nouveau rendu.",
                                  u"Argon muss neu gestartet werden, um zum neuen Renderer zu wechseln.",
                                  u"Argon deve essere riavviato per passare al nuovo renderer.", u"新しいレンダラーを使うには Argon の再起動が必要です。"};
const ViewerText kSavingSettings{u"Saving Settings", u"Sauvegarde des paramètres", u"Einstellungen werden gespeichert", u"Salvataggio impostazioni",
                                 u"設定をセーブ中"};
const ViewerText kDebug{u"Debug", u"Débogage", u"Debug", u"Debug", u"デバッグ"};
const ViewerText kDebugTicker{u"Open the track editor and debug tools.", u"Ouvre l'éditeur de circuits et les outils de débogage.",
                              u"Streckeneditor und Debug-Werkzeuge öffnen", u"Apri l'editor dei tracciati e gli strumenti di debug.",
                              u"トラックエディタとデバッグツールを開く"};
const ViewerText kExitWithProfile{u"Save your profile and return to the launcher.", u"Sauvegarde le profil et retourne au lanceur.",
                                  u"Profil speichern und zum Launcher zurückkehren", u"Salva il profilo e torna al launcher.",
                                  u"プレイヤーをセーブしてランチャーにもどる"};
const ViewerText kExitToLauncher{u"Return to the launcher.", u"Retourne au lanceur.", u"Zum Launcher zurückkehren", u"Torna al launcher.",
                                 u"ランチャーにもどる"};

const ViewerText kLanguageNames[kLanguageCount] = {
    {u"English", u"English", u"English", u"English", u"英語"},
    {u"Français", u"Français", u"Français", u"Français", u"フランス語"},
    {u"Deutsch", u"Deutsch", u"Deutsch", u"Deutsch", u"ドイツ語"},
    {u"Italiano", u"Italiano", u"Italiano", u"Italiano", u"イタリア語"},
    {u"Japanese", u"Japonais", u"Japanisch", u"Giapponese", u"日本語"},
};

namespace
{
const std::array<const ViewerText *, 79> kAll{
    &kSave.label,           &kSave.description,         &kLanguage.label,       &kLanguage.description,     &kRenderer.label,
    &kRenderer.description, &kWidescreen.label,         &kWidescreen.description, &kHudSize.label,          &kHudSize.description,
    &kResolution.label,     &kResolution.description,   &kFullscreen.label,     &kFullscreen.description,   &kVsync.label,
    &kVsync.description,    &kMsaa.label,               &kMsaa.description,     &kPostAa.label,             &kPostAa.description,
    &kTranslucency.label,   &kTranslucency.description, &kSpeedBlur.label, &kSpeedBlur.description, &kMarsBlurFix.label,
    &kMarsBlurFix.description, &kFmvs.label,           &kFmvs.description,         &kFpsMeter.label,
    &kFpsMeter.description, &kSpeakers.label,           &kSpeakers.description, &kHrtf.label,               &kHrtf.description,
    &kKemarTaps.label,      &kKemarTaps.description,    &kGameOrder,            &kOrderIndependent,         &kStereo,
    &kArgonOptions,         &kArgonOptionsTicker,       &kRendererRestart,      &kSavingSettings,           &kDebug,
    &kDebugTicker,          &kExitWithProfile,          &kExitToLauncher,       &kLanguageNames[0],         &kLanguageNames[1],
    &kLanguageNames[2],     &kLanguageNames[3],         &kLanguageNames[4],      &kGeneral.label,            &kGeneral.description,
    &kGraphics.label,       &kGraphics.description,     &kAudio.label,          &kAudio.description,        &kGameFixes.label,
    &kGameFixes.description, &kKeyBindings.label,       &kKeyBindings.description, &kKeyActions[0],         &kKeyActions[1],
    &kKeyActions[2],        &kKeyActions[3],            &kKeyActions[4],        &kKeyActions[5],            &kKeyActions[6],
    &kKeyActions[7],        &kKeyActions[8],            &kKeyActions[9],        &kKeyActions[10],           &kKeyActions[11],
    &kKeyActions[12],       &kKeyActions[13],           &kKeyBindingTicker,     &kPressAKey,                &kResetPageQuestion};
}  // namespace

std::span<const ViewerText *const> all()
{
    return kAll;
}

}  // namespace viewer_text
