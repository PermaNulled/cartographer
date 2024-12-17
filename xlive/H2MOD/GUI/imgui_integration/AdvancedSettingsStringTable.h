﻿#pragma once
#include "main/game_preferences.h"

/* enums */

enum e_advanced_string
{
	_advanced_string_title,
	_advanced_string_hud_title,
	_advanced_string_player_field_of_view,
	_advanced_string_default,
	_advanced_string_reset,
	_advanced_string_vehicle_field_of_view,
	_advanced_string_crosshair_offset,
	_advanced_string_crosshair_size,
	_advanced_string_hide_ingame_chat,
	_advanced_string_show_hud,
	_advanced_string_show_first_person,
	_advanced_string_weaponoffsets,
	_advanced_string_video_title,
	_advanced_string_fps_limit,
	_advanced_string_fps_limit_tooltip,
	_advanced_string_lod,
	_advanced_string_lod_1,
	_advanced_string_lod_2,
	_advanced_string_lod_3,
	_advanced_string_lod_4,
	_advanced_string_lod_5,
	_advanced_string_lod_6,
	_advanced_string_lod_tooltip,
	_advanced_string_tex_L1,
	_advanced_string_tex_L2,
	_advanced_string_tex_L3,
	_advanced_string_shadow_title,
	_advanced_string_water_title,
	_advanced_string_shader_lod_max,
	_advanced_string_shader_lod_max_tooltip,
	_advanced_string_light_suppressor,
	_advanced_string_light_suppressor_tooltip,
	_advanced_string_m_k_title,
	_advanced_string_raw_mouse,
	_advanced_string_raw_mouse_tooltip,
	_advanced_string_uniform_sensitivity,
	_advanced_string_uniform_sensitivity_tooltip,
	_advanced_string_raw_mouse_sensitivity,
	_advanced_string_mouse_sensitivity,
	_advanced_string_controller_title,
	_advanced_string_controller_sensitivity,
	_advanced_string_aiming_type,
	_advanced_string_modern,
	_advanced_string_aiming_type_tooltip,
	_advanced_string_deadzone_type,
	_advanced_string_axial,
	_advanced_string_radial,
	_advanced_string_both,
	_advanced_string_deadzone_type_tooltip,
	_advanced_string_axial_deadzone_X,
	_advanced_string_axial_deadzone_Y,
	_advanced_string_radial_deadzone_radius,
	_advanced_string_host_campagin_settings,
	_advanced_string_anti_cheat,
	_advanced_string_anti_cheat_tooltip,
	_advanced_string_disable_x_delay,
	_advanced_string_skull_anger,
	_advanced_string_skull_anger_tooltip,
	_advanced_string_skull_assassins,
	_advanced_string_skull_assassins_tooltip,
	_advanced_string_skull_black_eye,
	_advanced_string_skull_black_eye_tooltip,
	_advanced_string_skull_blind,
	_advanced_string_skull_blind_tooltip,
	_advanced_string_skull_catch,
	_advanced_string_skull_catch_tooltip,
	_advanced_string_skull_envy,
	_advanced_string_skull_envy_tooltip,
	_advanced_string_skull_famine,
	_advanced_string_skull_famine_tooltip,
	_advanced_string_skull_ghost,
	_advanced_string_skull_ghost_tooltip,
	_advanced_string_skull_grunt,
	_advanced_string_skull_grunt_tooltip,
	_advanced_string_skull_iron,
	_advanced_string_skull_iron_tooltip,
	_advanced_string_skull_iwbyd,
	_advanced_string_skull_iwbyd_tooltip,
	_advanced_string_skull_mythic,
	_advanced_string_skull_mythic_tooltip,
	_advanced_string_skull_sputnik,
	_advanced_string_skull_sputnik_tooltip,
	_advanced_string_skull_thunderstorm,
	_advanced_string_skull_thunderstorm_tooltip,
	_advanced_string_skull_whuppopotamus,
	_advanced_string_skull_whuppopotamus_tooltip,
	_advanced_string_game_title,
	_advanced_string_discord_presence,
	_advanced_string_disable_intro_videos,
	_advanced_string_language,
	_advanced_string_lang_english,
	_advanced_string_lang_japanese,
	_advanced_string_lang_german,
	_advanced_string_lang_french,
	_advanced_string_lang_spanish,
	_advanced_string_lang_italian,
	_advanced_string_lang_korean,
	_advanced_string_lang_chinese,
	_advanced_string_lang_native,
	_advanced_string_static_fp,
	_advanced_string_static_fp_tooltip,
	_advanced_string_upnp_title,
	_advanced_string_upnp_tooltip,
	_advanced_string_no_events_title,
	_advanced_string_no_events_tooltip,
	_advanced_string_skeleton_biped,
	_advanced_string_skeleton_biped_tooltip,
	k_advanced_string_count
};

/* constants */

const char *const k_advanced_settings_string_table_english[k_advanced_string_count]
{
	"        Advanced Settings",
	"HUD Settings",
	"Player Field of View",
	"Default",
	"Reset",
	"Vehicle Field of View",
	"Crosshair Offset",
	"Crosshair Size",
	"Hide Ingame Chat",
	"Show HUD",
	"Show First Person",
	"Adjust Weapon Offsets",
	"Video Settings",
	"FPS Limit",
	"Setting this to 0 will uncap the game's frame rate.",
	"Level of Detail",
	"L1 - Very Low",
	"L2 - Low",
	"L3 - Medium",
	"L4 - High",
	"L5 - Very High",
	"L6 - Cinematic",
	"Changing this will force the game to use the set Level of Detail for models that have them\nLeaving it at default makes it dynamic which is the games default behaviour.",
	"Low",
	"High",
	"Ultra",
	"Shadow Quality",
	"Water Quality",
	"Force Max Shader LOD",
	"This will force shaders to use the highest LODS regardless of their distance away from the player\nA restart is required for these changes to take effect.",
	"Disable Light Suppression",
	"This will force lights to not fade out when there's multiple of them onscreen\nA restart is required for these changes to take effect.",
	"Mouse and Keyboard Input",
	"Raw Mouse Input",
	"This will remove the game's default mouse acceleration.",
	"Uniform Sensitivity",
	"By default the game has the vertical sensitivity half of the horizontal.\nEnabling this option will make these match.",
	"Raw Mouse Sensitivity",
	"Mouse Sensitivity",
	"Controller Input",
	"Controller Sensitivity",
	"Aiming Type",
	"Modern",
	"Modern Aiming will remove the native acceleration zones from a controller while aiming, allowing for a more precise aim.\n\nNOTE: Selecting Modern Aiming will cause Raw Mouse input to not work.",
	"Deadzone Type",
	"Axial",
	"Radial",
	"Both",
	"Halo 2 by default uses axial deadzones, radial deadzones have been added as another option for players.",
	"Axial Deadzone X",
	"Axial Deadzone Y",
	"Radial Deadzone Radius",
	"Host & Campaign Settings",
	"Anti-Cheat",
	"Allows you to disable the Anti-Cheat for your lobby.",
	"Enable X to Delay",
	"Anger",
	"Enemies and allies fire their weapons faster and more frequently.",
	"Assassins",
	"All enemies in game are permanently cloaked. Allies can sometimes\nsee them but mostly they can't, so they can't help much.",
	"Black Eye",
	"Your shield does not charge normally. To charge your shields you\nmust kill something with a melee attack",
	"Blind",
	"Your heads-up display becomes invisible. In other words, you cannot\nsee your weapon, body, shields, ammunition, motion tracker,\n or use your flashlight.",
	"Catch",
	"A.I. will throw more grenades. Also, everybody will drop two grenades\n of their kind Flood will drop grenades depending on whether\n they're human or Covenant.",
	"Envy",
	"The Master Chief now has an Active camouflage just like the Arbiter's.\nHowever, there is no visible timer, so remember: five second\n cloak with ten second recharge on Legendary",
	"Famine",
	"All dropped weapons have half ammo. Weapons that spawned on the floor or\nspawned with are unaffected.",
	"Ghost",
	"A.I. characters will not flinch from attacks, melee or otherwise.",
	"Grunt Birthday",
	"Headshots turn into Plasma Grenade explosions.",
	"Iron",
	"When playing co-op, if either player dies the game restarts you at your\nlast checkpoint.",
	"IWBYD",
	"The rarity of combat dialog is changed, rare lines become far more common\nbut common lines are still present at their normal rate",
	"Mythic",
	"Enemies have more health and shielding, and are therefore harder to kill.",
	"Sputnik",
	"The mass of certain objects is severely reduced, making them fly further""\nwhen smacked with a melee hit, or when they are near an explosion",
	"Thunderstorm",
	"Causes most enemy and ally units to be their highest rank.",
	"Whuppopotamus",
	"Strengthens the hearing of both allies and enemies",
	"Game Settings",
	"Discord Rich Presence",
	"Disable Intro videos",
	"Language",
	"English",
	"Japanese",
	"German",
	"French",
	"Spanish",
	"Italian",
	"Korean",
	"Chinese",
	"Native",
	"Static FP Scale",
	"This setting will force your First person model to stay the default size independent of FOV.",
	"UPNP Enabled",
	"Enabled UPNP Port forwarding for the project.",
	"No Events",
	"Opt out of event cosmetics restart required to take effect",
	"Play as Spooky boy",
	"Changes your biped to be a Spooky Scary Skeleton for the Halloween event"
};

// Make sure that the spanish strings include u8 at the front of them or else they wont display properly

const char *const k_advanced_settings_string_table_spanish[k_advanced_string_count]
{
	u8"      Ajustes avanzados",
	u8"Ajustes de Interfaz",
	u8"Campo de visión (Personaje)",
	u8"Inicial",
	u8"Inicial",
	u8"Campo de visión (Vehículo)",
	u8"Posición de la mira",
	u8"Tamaño de la mira",
	u8"Ocultar chat en partida",
	u8"Mostrar Interfaz",
	u8"Mostrar primera persona",
	u8"Ajustar compensaciones de armas",
	u8"Ajustes de video",
	u8"Limitar FPS",
	u8"Dejar este ajuste en 0 quitará el límite de fotogramas por segundo.",
	u8"Nivel de detalle",
	u8"N1 - Muy bajo",
	u8"N2 - Bajo",
	u8"N3 - Medio",
	u8"N4 - Alto",
	u8"N5 - Muy alto",
	u8"N6 - Cinemático",
	u8"Cambiar esto forzará el juego a usar los modelos del nivel de detalle seleccionado si están disponibles.\nDejarlo en Predeterminado hará que el nivel de detalle sea dinámico y controlado por el juego.",
	u8"Bajo",
	u8"Alto",
	u8"Muy alto",
	u8"Calidad sombra",
	u8"Calidad del agua",
	u8"Máximo nivel de profundidad de sombreado",
	u8"Esto obligará a los shaders a usar los LODS más altos independientemente de su distancia del jugador.\nEste ajuste requiere reiniciar el juego para que tenga efecto.",
	u8"Desactivar supresión de luz",
	u8"Esto hará que las luces no se desvanezcan cuando haya varias en la pantalla\nEste ajuste requiere reiniciar el juego para que tenga efecto.",
	u8"Entrada de mouse y teclado",
	u8"Entrada de mouse pura",
	u8"Esto desactivará la aceleración de mouse predeterminada del juego.",
	u8"Sensibilidad uniforme",
	u8"Por defecto el juego tiene la sensibilidad vertical a la mitad de la horizontal.\nActivar esta opción igualará estas sensibilidades.",
	u8"Sensibilidad de mouse pura",
	u8"Sensibilidad de mouse",
	u8"Entrada de mando",
	u8"Sensibilidad de mando",
	u8"Tipo de apuntado",
	u8"Moderno",
	u8"El apuntado Moderno eliminará las zonas de aceleración por defecto del mando al apuntar, lo que permite un apuntado más preciso.\n\nNOTA: Seleccionar apuntado Moderno hará que la Entrada de mouse pura no funcione.",
	u8"Tipo de Zona muerta",
	u8"Por eje",
	u8"Radial",
	u8"Ambos",
	u8"Por defecto, Halo 2 usa zonas muertas axiales. Las zonas muertas radiales fueron agregadas como otra opción para los jugadores.",
	u8"Zona muerta del Eje X",
	u8"Zona muerta del Eje Y",
	u8"Radio de zona muerta radial",
	u8"Ajustes de anfitrión y campaña",
	u8"Anti-Trampas",
	u8"Permite desactivar el anti-trampas de tu sala.",
	u8"Desactivar X para retrasar",
	u8"Ira",
	u8"Los enemigos y aliados disparan sus armas de forma más rápida y frecuente.",
	u8"Asesinos",
	u8"Todos los enemigos en la partida serán camuflados permanentemente. Los aliados algunas veces\nlos verán pero no será muy frecuente, por esto no podrán ayudarte mucho.",
	u8"Ojo Morado",
	u8"Tu escudo no se recarga normalmente. Para recargar tu escudo\ndebes matar algo con un ataque cuerpo a cuerpo. ",
	u8"Ciego",
	u8"Tu interfaz en partida se hace invisible. En otras palabras, no podrás\nver tu arma, cuerpo, escudos, munición, sensor de movimiento,\no usar tu linterna.",
	u8"Pilla",
	u8"La I.A. arrojará más granadas. Además, todos dejarán 2 granadas\ncorrespondientes. Los Flood dejarán granadas dependiendo\nsi son humanos o Covenant.",
	u8"Envidia",
	u8"El Jefe Maestro ahora tiene un Camuflaje activo al igual que El Árbitro.\nSin embargo, no verás el cronómetro, así que recuerda: cinco segundos\n de camuflaje con diez segundos de recarga en dificultad Legendario.",
	u8"Hambruna",
	u8"Todas las armas que dejen al morir tienen la mitad de la munición. Las armas que aparecen en el mapa\n no serán afectadas.",
	u8"Fantasma",
	u8"Los personajes de I.A. no retrocederán a los ataques, ya sean cuerpo a cuerpo u otro tipo.",
	u8"Cumpleaños Grunt",
	u8"Los tiros a la cabeza se convierten en explosiones de Granada de Plasma.",
	u8"Hierro",
	u8"Al jugar co-operativo, si alguno de los jugadores muere el juego volverá al\núltimo punto de control.",
	u8"IWBYD",
	u8"La rareza del diálogo de combate cambiará, el diálogo raro será más frecuente\n pero el diálogo común seguirá escuchándose con la misma frecuencia.",
	u8"Mítico",
	u8"Los enemigos tienen más salud y escudo, así que serán más difíciles de matar.",
	u8"Sputnik",
	u8"La masa de ciertos objetos será muy reducida, haciendo que vuelen más lejos\nsi son golpeados por un ataque cuerpo a cuerpo o si están cerca de una explosión.",
	u8"Tormenta Eléctrica",
	u8"La mayoría de unidades enemigas y aliadas serán del rango más alto.",
	u8"Whuppopotamus",
	u8"Mejora el oído de aliados y enemigos.",
	u8"Ajustes del juego",
	u8"Discord Rich Presence",
	u8"Desactivar Videos al inicio",
	u8"Idioma",
	u8"Inglés",
	u8"Japonés",
	u8"Alemán",
	u8"Francés",
	u8"Español",
	u8"Italiano",
	u8"Coreano",
	u8"Chino",
	u8"Nativo",
	u8"Escala FP estática",
	u8"Esta configuración obligará a su modelo en primera persona a mantener el tamaño predeterminado\nindependientemente del campo de visión.",
	u8"UPNP habilitado",
	u8"Habilita el reenvío de puertos UPNP para el proyecto.",
	u8"No hay eventos",
	u8"Se requiere el reinicio de los cosméticos del evento para que surta efecto",
	u8"Juega como Spooky boy",
	u8"Cambia a tu bípedo para que sea un esqueleto espeluznante y aterrador para el evento de Halloween."
};

// TODO: finish localization
const char *const *const k_advanced_settings_string_table[k_language_count] =
{
	k_advanced_settings_string_table_english,
	k_advanced_settings_string_table_english,
	k_advanced_settings_string_table_english,
	k_advanced_settings_string_table_english,
	k_advanced_settings_string_table_spanish,
	k_advanced_settings_string_table_english,
	k_advanced_settings_string_table_english,
	k_advanced_settings_string_table_english,
	k_advanced_settings_string_table_english
};
