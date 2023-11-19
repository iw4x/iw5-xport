#include <std_include.hpp>
#include <loader/module_loader.hpp>

#include "game/game.hpp"

#include <utils/hook.hpp>
#include <utils/io.hpp>
#include <utils/flags.hpp>

#include <module/teams_exporter.hpp>
#include <module/command.hpp>

#include <module/asset_dumpers/igfximage.hpp>
#include <module/asset_dumpers/imaterial.hpp>
#include <module/asset_dumpers/itechniqueset.hpp>
#include <module/asset_dumpers/ixmodel.hpp>
#include <module/asset_dumpers/iphyspreset.hpp>
#include <module/asset_dumpers/igfxworld.hpp>
#include <module/asset_dumpers/iglassworld.hpp>
#include <module/asset_dumpers/icomworld.hpp>
#include <module/asset_dumpers/ilightdef.hpp>
#include <module/asset_dumpers/iscriptfile.hpp>
#include <module/asset_dumpers/irawfile.hpp>
#include <module/asset_dumpers/imapents.hpp>
#include <module/asset_dumpers/iclipmap.hpp>
#include <module/asset_dumpers/isndalias.hpp>
#include <module/asset_dumpers/isndcurve.hpp>
#include <module/asset_dumpers/iloadedsound.hpp>
#include <module/asset_dumpers/ifxworld.hpp>
#include <module/asset_dumpers/ifx.hpp>
#include <module/asset_dumpers/ixanimparts.hpp>
#include <module/asset_dumpers/itracerdef.hpp>
#include <module/asset_dumpers/iweapon.hpp>

#include <module/scheduler.hpp>
#include <module/log_file.hpp>
#include <module/console.hpp>

#include "exporter.hpp"
#include "asset_dumper.hpp"

const game::native::dvar_t* exporter::export_path_dvar;
const game::native::dvar_t* exporter::export_teams_dvar;

asset_dumper* exporter::asset_dumpers[game::native::ASSET_TYPE_COUNT]{};
std::vector<std::string> exporter::captured_scripts{};
std::vector<std::string> exporter::captured_rawfiles{};
std::vector<std::string> exporter::captured_snd{};
std::vector<std::string> exporter::captured_models{};
std::vector<std::string> exporter::captured_fx{};
bool exporter::capture = false;
bool exporter::ready = false;
std::string exporter::map_name{};
std::vector<std::string> exporter::prepared_source{};
std::unordered_map<std::string, std::string> exporter::rawfile_rename_map{};
std::unordered_map<iw4::native::XAssetType, game::native::XAssetType> exporter::iw4_to_iw5_type_table =
{
		{ iw4::native::ASSET_TYPE_PHYSPRESET, game::native::ASSET_TYPE_PHYSPRESET },
		{ iw4::native::ASSET_TYPE_PHYSCOLLMAP, game::native::ASSET_TYPE_PHYSCOLLMAP },
		{ iw4::native::ASSET_TYPE_XANIMPARTS, game::native::ASSET_TYPE_XANIMPARTS },
		{ iw4::native::ASSET_TYPE_XMODEL_SURFS, game::native::ASSET_TYPE_XMODEL_SURFS },
		{ iw4::native::ASSET_TYPE_XMODEL, game::native::ASSET_TYPE_XMODEL },
		{ iw4::native::ASSET_TYPE_MATERIAL, game::native::ASSET_TYPE_MATERIAL },
		{ iw4::native::ASSET_TYPE_PIXELSHADER, game::native::ASSET_TYPE_PIXELSHADER },
		{ iw4::native::ASSET_TYPE_VERTEXSHADER, game::native::ASSET_TYPE_VERTEXSHADER },
		{ iw4::native::ASSET_TYPE_VERTEXDECL, game::native::ASSET_TYPE_VERTEXDECL },
		{ iw4::native::ASSET_TYPE_TECHNIQUE_SET, game::native::ASSET_TYPE_TECHNIQUE_SET },
		{ iw4::native::ASSET_TYPE_IMAGE, game::native::ASSET_TYPE_IMAGE },
		{ iw4::native::ASSET_TYPE_SOUND, game::native::ASSET_TYPE_SOUND },
		{ iw4::native::ASSET_TYPE_SOUND_CURVE, game::native::ASSET_TYPE_SOUND_CURVE },
		{ iw4::native::ASSET_TYPE_LOADED_SOUND, game::native::ASSET_TYPE_LOADED_SOUND },
		{ iw4::native::ASSET_TYPE_CLIPMAP_SP, game::native::ASSET_TYPE_CLIPMAP },
		{ iw4::native::ASSET_TYPE_CLIPMAP_MP, game::native::ASSET_TYPE_CLIPMAP },
		{ iw4::native::ASSET_TYPE_COMWORLD, game::native::ASSET_TYPE_COMWORLD },
		{ iw4::native::ASSET_TYPE_GAMEWORLD_SP, game::native::ASSET_TYPE_GLASSWORLD },
		{ iw4::native::ASSET_TYPE_GAMEWORLD_MP, game::native::ASSET_TYPE_GLASSWORLD },
		{ iw4::native::ASSET_TYPE_MAP_ENTS, game::native::ASSET_TYPE_MAP_ENTS },
		{ iw4::native::ASSET_TYPE_FXWORLD, game::native::ASSET_TYPE_FXWORLD },
		{ iw4::native::ASSET_TYPE_GFXWORLD, game::native::ASSET_TYPE_GFXWORLD },
		{ iw4::native::ASSET_TYPE_LIGHT_DEF, game::native::ASSET_TYPE_LIGHT_DEF },
		{ iw4::native::ASSET_TYPE_UI_MAP, game::native::ASSET_TYPE_UI_MAP },
		{ iw4::native::ASSET_TYPE_FONT, game::native::ASSET_TYPE_FONT },
		{ iw4::native::ASSET_TYPE_MENULIST, game::native::ASSET_TYPE_MENULIST },
		{ iw4::native::ASSET_TYPE_MENU, game::native::ASSET_TYPE_MENU },
		{ iw4::native::ASSET_TYPE_LOCALIZE_ENTRY, game::native::ASSET_TYPE_LOCALIZE_ENTRY },
		{ iw4::native::ASSET_TYPE_WEAPON, game::native::ASSET_TYPE_WEAPON },
		{ iw4::native::ASSET_TYPE_SNDDRIVER_GLOBALS, game::native::ASSET_TYPE_SNDDRIVER_GLOBALS },
		{ iw4::native::ASSET_TYPE_FX, game::native::ASSET_TYPE_FX },
		{ iw4::native::ASSET_TYPE_IMPACT_FX, game::native::ASSET_TYPE_IMPACT_FX },
		{ iw4::native::ASSET_TYPE_AITYPE, game::native::ASSET_TYPE_AITYPE },
		{ iw4::native::ASSET_TYPE_MPTYPE, game::native::ASSET_TYPE_MPTYPE },
		{ iw4::native::ASSET_TYPE_CHARACTER, game::native::ASSET_TYPE_CHARACTER },
		{ iw4::native::ASSET_TYPE_XMODELALIAS, game::native::ASSET_TYPE_XMODELALIAS },
		{ iw4::native::ASSET_TYPE_RAWFILE, game::native::ASSET_TYPE_RAWFILE },
		{ iw4::native::ASSET_TYPE_STRINGTABLE, game::native::ASSET_TYPE_STRINGTABLE },
		{ iw4::native::ASSET_TYPE_LEADERBOARD, game::native::ASSET_TYPE_LEADERBOARD },
		{ iw4::native::ASSET_TYPE_STRUCTURED_DATA_DEF, game::native::ASSET_TYPE_STRUCTURED_DATA_DEF },
		{ iw4::native::ASSET_TYPE_TRACER, game::native::ASSET_TYPE_TRACER },
		{ iw4::native::ASSET_TYPE_VEHICLE, game::native::ASSET_TYPE_VEHICLE },
		{ iw4::native::ASSET_TYPE_ADDON_MAP_ENTS, game::native::ASSET_TYPE_ADDON_MAP_ENTS },
		{ iw4::native::ASSET_TYPE_COUNT, game::native::ASSET_TYPE_COUNT },
		{ iw4::native::ASSET_TYPE_STRING, game::native::ASSET_TYPE_STRING },
		{ iw4::native::ASSET_TYPE_ASSETLIST, game::native::ASSET_TYPE_ASSETLIST },

};

iw4of::api* exporter::iw4of_api{};
std::string exporter::common_sounds[] = {
	"rocket_explode_bark",
	"rocket_explode_brick",
	"rocket_explode_carpet",
	"rocket_explode_cloth",
	"rocket_explode_flesh",
	"rocket_explode_foliage",
	"rocket_explode_mud",
	"rocket_explode_paper",
	"rocket_explode_plaster",
	"rocket_explode_sand",
	"rocket_explode_wood",
	"rocket_explode_ceramic",
	"rocket_explode_plastic",
	"rocket_explode_rubber",
	"rocket_explode_cushion",
	"rocket_explode_fruit",
	"rocket_explode_paintedmetal",
	"rocket_explode_riotshield",
	"step_walk_plr_dirt",
	"step_walk_plr_metal",
	"step_walk_plr_sand",
	"step_walk_plr_snow",
	"fire_vehicle_flareup_med",
	"fire_vehicle_med",
	"physics_tire_default"
};


typedef bool (*DB_Update_t)();
DB_Update_t DB_Update = (DB_Update_t)0x4CDA40;

typedef bool (*SV_Map_f_t)(int a1, const char* mapName, unsigned __int8 isPreloaded, unsigned __int8 migrate);
SV_Map_f_t SV_Map_f = (SV_Map_f_t)0x56EEA0;

typedef void (*Cbuf_Execute_t)(int localClient, int controllerIndex);
Cbuf_Execute_t Cbuf_Execute = (Cbuf_Execute_t)0x546590;

DEFINE_OG_FUNCTION(Com_EventLoop, 0x555880);

DEFINE_OG_FUNCTION(R_RegisterDvars, 0X6678F0);
DEFINE_OG_FUNCTION(Com_Frame, 0x556870);
DEFINE_OG_FUNCTION(Sys_Quit, 0x5CD0E0);

DEFINE_OG_FUNCTION(SL_Init, 0x5643A0);
DEFINE_OG_FUNCTION(Swap_Init, 0x5C2800);
DEFINE_OG_FUNCTION(Cbuf_Init, 0x545640);
DEFINE_OG_FUNCTION(Cmd_Init, 0x546710);
DEFINE_OG_FUNCTION(Com_InitDvars, 0x5540F0);
DEFINE_OG_FUNCTION(Com_InitHunkMemory, 0x5B6990);
DEFINE_OG_FUNCTION(Scr_InitProgramHunkUser, 0x561AC0);
DEFINE_OG_FUNCTION(LargeLocalInit, 0x5B75B0);
DEFINE_OG_FUNCTION(Con_OneTimeInit, 0x487120);
DEFINE_OG_FUNCTION(Sys_InitCmdEvents, 0x55F730);
DEFINE_OG_FUNCTION(PMem_Init, 0x5C13D0);
DEFINE_OG_FUNCTION(Com_GetInit, 0x556420);

DEFINE_OG_FUNCTION(j_R_InitWorkerThrea, 0x690FF0);
DEFINE_OG_FUNCTION(CL_InitKeyCommands, 0x48C560);
DEFINE_OG_FUNCTION(FS_InitFilesystem, 0x5B20A0);
DEFINE_OG_FUNCTION(Con_InitChannels, 0X49AB60);
DEFINE_OG_FUNCTION(LiveStorage_Init, 0x550520);

DEFINE_OG_FUNCTION(DB_InitThread, 0x4CD7C0);
DEFINE_OG_FUNCTION(Com_InitPreXAssets, 0x5545D0);
DEFINE_OG_FUNCTION(Live_Init, 0x5CB170);
DEFINE_OG_FUNCTION(Content_Init, 0x5BC580);
DEFINE_OG_FUNCTION(Sys_Init, 0x5CC8A0);
DEFINE_OG_FUNCTION(Scr_InitVariables, 0x5650E0);
DEFINE_OG_FUNCTION(Scr_Init, 0x568F40);

DEFINE_OG_FUNCTION(CL_InitOnceForAllClients, 0x487B00);
DEFINE_OG_FUNCTION(Com_AddStartupCommands, 0x5557F0);

void exporter::event_loop()
{
	while (true)
	{
		Com_Frame();
		Com_EventLoop();

		if (ready)
		{
			Cbuf_Execute(0, 0);
		}

		Sleep(1u);
	}
}

void Com_ParseCommandLine(const char* commandLineArgs)
{
	static const auto Com_ParseCommandLine_t = 0x553B60;

	__asm
	{
		pushad
		mov eax, commandLineArgs
		call Com_ParseCommandLine_t
		popad
	}
}

void exporter::perform_common_initialization()
{
	SL_Init();
	Swap_Init();
	Cbuf_Init();
	Cmd_Init();
	Com_InitDvars();
	Com_InitHunkMemory();
	Scr_InitProgramHunkUser();
	LargeLocalInit();
	Con_OneTimeInit();
	Sys_InitCmdEvents();
	PMem_Init();

	j_R_InitWorkerThrea();
	CL_InitKeyCommands();
	FS_InitFilesystem();
	Con_InitChannels();

	DB_InitThread();
	Com_InitPreXAssets();

	Live_Init();
	Content_Init();

	Sys_Init();
	Scr_InitVariables();
	Scr_Init();

	CL_InitOnceForAllClients();

	// Manual "renderer" init
	R_RegisterDvars();
	auto renderer = game::native::Dvar_FindVar("r_loadForRenderer");
	memset(&renderer->modified, 0, sizeof game::native::dvar_t - 9);
	//

	auto com_frameTime = (int*)(0x1CF0B88);
	*com_frameTime = 1;

	auto com_fullyInitialized = (bool*)(0x1CF0BB8);
	*com_fullyInitialized = 1;
	load_common_zones();

	export_path_dvar = game::native::Dvar_RegisterString("export_path", "iw5xport_out/default", game::native::DVAR_NONE, "export path for iw5xport");
	export_teams_dvar = game::native::Dvar_RegisterBool("export_team_data", true, game::native::DVAR_NONE, "should we xport team bodies?");

	auto cmdLine = GetCommandLineA();
	Com_ParseCommandLine(cmdLine);

	initialize_exporters();
	add_commands();


	DB_Update();
	Com_AddStartupCommands();

	Com_EventLoop();
	Cbuf_Execute(0, 0);
}

void exporter::dump_teams(const command::params& params)
{
	if (params.size() < 2) return;
	map_name = params[1];

	command::execute(std::format("loadzone {}", map_name), true);

	while (!DB_Update())
	{
		Sleep(1u);
	}
	
	// UI_LoadArenasFromFile_FastFile
	utils::hook::invoke<void>(0x589A20);

	unsigned int ui_numArenas = *reinterpret_cast<unsigned int*>(0x58DF500);
	char** ui_arenaInfos = reinterpret_cast<char**>(0x58DF400);

	bool found = false;
	std::string teams[2];

	for (size_t i = 0; i < ui_numArenas; i++)
	{
		const auto map = game::native::Info_ValueForKey(ui_arenaInfos[i], "map");
		if (map == map_name)
		{
			teams[0] = game::native::Info_ValueForKey(ui_arenaInfos[i], "allieschar");
			teams[1] = game::native::Info_ValueForKey(ui_arenaInfos[i], "axischar");
			found = true;
			break;
		}
	}

	if (!found)
	{
		console::error("Could not find arena for {}\n", map_name.data());
		return;
	}

	for (size_t i = 0; i < 2; i++)
	{
		const auto fmt = std::format("iw5xport_out/{}", teams[i]);
		game::native::Dvar_SetString(export_path_dvar, fmt.data());
		iw4of_api->clear_writes();
		iw4of_api->set_work_path(export_path_dvar->current.string);
		prepared_source.clear();

		teams_exporter::export_team(teams[i]);
	}
}

void exporter::dump_map(const command::params& params)
{
	if (params.size() < 2) return;
	map_name = params[1];

	iw4of_api->clear_writes();
	iw4of_api->set_work_path(export_path_dvar->current.string);

	console::info("dumping %s...\n", map_name.c_str());

	command::execute(std::format("loadzone {}_load", map_name), true);

	capture = true;
	command::execute(std::format("loadzone {}", map_name), true);

	while (!DB_Update())
	{
		Sleep(1u);
	}

	prepared_source.clear();
	capture = false;

	console::info("dumping fxworld %s...\n", map_name.c_str());
	command::execute("dumpFxWorld", true); // This adds it to the zone source

	prepared_source.emplace_back("\n");
	console::info("dumping comworld %s...\n", map_name.c_str());
	command::execute("dumpComWorld", true); // This adds it to the zone source

	prepared_source.emplace_back("\n");
	console::info("dumping glassworld %s...\n", map_name.c_str());
	command::execute("dumpGlassWorld", true); // This adds it to the zone source

	prepared_source.emplace_back("\n");
	console::info("dumping clipmap %s...\n", map_name.c_str());
	command::execute("dumpClipMap", true); // This adds it to the zone source

	prepared_source.emplace_back("\n");
	console::info("dumping gfxworld %s...\n", map_name.c_str());
	command::execute("dumpGfxWorld", true); // This adds it to the zone source

	for (const auto& script : exporter::captured_scripts)
	{
		if (
			!script.starts_with("maps/mp/gametypes"s) // No general gameplay scripts
			)
		{
			console::info("dumping script %s...\n", script.data());
			command::execute(std::format("dumpScript {}", script), true); // This adds it to the zone source
		}
	}

	for (const auto& rawfile : exporter::captured_rawfiles)
	{
		if (rawfile == std::format("{}_load", map_name) || rawfile == map_name)
		{
			// Idk what this is, it gets captured but not dumped. Looks like an empty asset
			// IW branding?
			continue;
		}

		if (rawfile.ends_with(".gsc"))
		{
			// => scriptfiles
			continue;
		}

		console::info("dumping rawfile %s...\n", rawfile.data());
		command::execute(std::format("dumpRawFile {}", rawfile), true); // This adds it to the zone source
	}

	auto script_exporter = reinterpret_cast<asset_dumpers::iscriptfile*>(asset_dumpers[game::native::XAssetType::ASSET_TYPE_SCRIPTFILE]);
	if (script_exporter)
	{
		console::info("dumping common scripts, this can take a while...\n");
		script_exporter->dump_rename_common_scripts();
	}

	captured_rawfiles.clear();
	captured_scripts.clear();

	prepared_source.emplace_back("\n");
	command::execute(std::format("dumpScript maps/mp/{}", map_name), true);
	command::execute(std::format("dumpScript maps/mp/{}_precache", map_name), true);
	command::execute(std::format("dumpScript maps/createart/{}_art", map_name), true);

	prepared_source.emplace_back("\n");
	command::execute(std::format("dumpScript maps/mp/{}_fx", map_name), true);
	command::execute(std::format("dumpScript maps/createfx/{}_fx", map_name), true);
	command::execute(std::format("dumpScript maps/createart/{}_fog", map_name), true);

	prepared_source.emplace_back("\n");
	command::execute("dumpRawFile animtrees/animated_props.atr", true);

	prepared_source.emplace_back("\n");
	console::info("Exporting Vision...\n");
	command::execute(std::format("dumpRawFile vision/{}.vision", map_name.data()), true);

	console::info("Exporting Sun...\n");
	command::execute(std::format("dumpRawFile sun/{}.sun", map_name.data()), true);

	prepared_source.emplace_back("\n");
	console::info("Exporting Compass...\n");
	command::execute(std::format("dumpMaterial compass_map_{}", map_name.data()), true);

	console::info("Exporting Loadscreen...\n");
	command::execute(std::format("dumpGfxImage loadscreen_{}", map_name.data()), true);
	command::execute("dumpMaterial $levelbriefing", true);

	console::info("Generic sounds...\n");
	prepared_source.emplace_back("\n# Generic sounds\n");
	for (size_t i = 0; i < ARRAYSIZE(common_sounds); i++)
	{
		command::execute(std::format("dumpSound {}", common_sounds[i]), true);
	}

	console::info("Additional fluff...\n");
	prepared_source.emplace_back("\n# Additional fluff (mostly destructibles)\n");

	for (const auto& fx : exporter::captured_fx)
	{
		command::execute(std::format("dumpFX {}", fx), true); // This adds it to the zone source
	}
	for (const auto& model : exporter::captured_models)
	{
		command::execute(std::format("dumpXModel {}", model), true); // This adds it to the zone source
	}

	for (const auto& snd : exporter::captured_snd)
	{
		command::execute(std::format("dumpSound {}", snd), true); // This adds it to the zone source
	}

	captured_models.clear();
	captured_snd.clear();
	captured_fx.clear();


	console::info("Writing source...\n");

	utils::io::write_file(std::format("{}/{}.csv", export_path_dvar->current.string, map_name), get_source(), false);
	utils::io::write_file(std::format("{}/{}_load.csv", export_path_dvar->current.string, map_name), "material,$levelbriefing\n", false);

	console::info("done!\n");

	// Clear memory of everybody
	initialize_exporters();
	prepared_source.clear();
}

void exporter::add_commands()
{
	command::add("quit", []()
		{
			// Do not de-init renderer and zones and stuff
			Sys_Quit();
		});

	command::add("dumpteams", dump_teams);

	command::add("dumpmap", dump_map);

	command::add("loadzone", [](const command::params& params)
		{
			if (params.size() < 2) return;
			std::string zone = params[1];

			game::native::XZoneInfo info;
			info.name = zone.data();
			info.allocFlags = 0;
			info.freeFlags = 0;

			game::native::DB_LoadXAssets(&info, 1, 0);
			console::info("successfully loaded %s!\n", info.name);
		});
}

void exporter::load_common_zones()
{
	const std::string common_zones[] = {
		"code_post_gfx_mp",
		"localized_code_post_gfx_mp",

		// If we include these we can't loadzone some maps anymore, like mp_underground
		//"ui_mp",
		//"localized_ui_mp",

		"patch_mp",
		"common_mp",
		"localized_common_mp",
	};

	console::info("loading common zones...\n");

	for (auto& zone : common_zones)
	{
		game::native::XZoneInfo info;
		info.name = zone.data();
		info.allocFlags = 1;
		info.freeFlags = 0;

		game::native::DB_LoadXAssets(&info, 1, 0);
	}


	console::info("done!\n");
}

void exporter::initialize_exporters()
{
	for (auto dumper : asset_dumpers)
	{
		if (dumper)
		{
			delete dumper;
		}
	}

	asset_dumpers[game::native::XAssetType::ASSET_TYPE_IMAGE] = new asset_dumpers::igfximage();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_MATERIAL] = new asset_dumpers::imaterial();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_TECHNIQUE_SET] = new asset_dumpers::itechniqueset();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_PHYSPRESET] = new asset_dumpers::iphyspreset();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_XMODEL] = new asset_dumpers::ixmodel();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_GFXWORLD] = new asset_dumpers::igfxworld();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_GLASSWORLD] = new asset_dumpers::iglassworld();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_COMWORLD] = new asset_dumpers::icomworld();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_LIGHT_DEF] = new asset_dumpers::ilightdef();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_SCRIPTFILE] = new asset_dumpers::iscriptfile();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_RAWFILE] = new asset_dumpers::irawfile();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_MAP_ENTS] = new asset_dumpers::imapents();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_CLIPMAP] = new asset_dumpers::iclipmap();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_SOUND] = new asset_dumpers::isndalias();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_LOADED_SOUND] = new asset_dumpers::iloadedsound();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_SOUND_CURVE] = new asset_dumpers::isndcurve();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_FXWORLD] = new asset_dumpers::ifxworld();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_FX] = new asset_dumpers::ifx();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_XANIMPARTS] = new asset_dumpers::ixanimparts();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_WEAPON] = new asset_dumpers::iweapon();
	asset_dumpers[game::native::XAssetType::ASSET_TYPE_TRACER] = new asset_dumpers::itracerdef();
}

bool exporter::exporter_exists(game::native::XAssetType assetType)
{
	return asset_dumpers[assetType];
}

iw4::native::XAssetHeader exporter::convert_and_write(game::native::XAssetType type, game::native::XAssetHeader header)
{
	if (exporter_exists(type))
	{
		return asset_dumpers[type]->convert_and_write(header);
	}
	else
	{
		console::warn("Cannot dump type %s, no asset handler found\n", game::native::g_assetNames[type]);
		return iw4::native::XAssetHeader{};
	}
}

iw4::native::XAssetHeader exporter::convert(game::native::XAssetType type, game::native::XAssetHeader header)
{
	if (exporter_exists(type))
	{
		return asset_dumpers[type]->convert(header);
	}
	else
	{
		console::warn("Cannot convert type %s, no asset handler found\n", game::native::g_assetNames[type]);
		return iw4::native::XAssetHeader{};
	}
}

void exporter::add_to_source(game::native::XAssetType type, const std::string asset)
{
	auto name = game::native::g_assetNames[type];

	if (type == game::native::ASSET_TYPE_GLASSWORLD)
	{
		name = "game_map_mp"; // Special case for iw4
	}

	assert(type != game::native::ASSET_TYPE_SCRIPTFILE);

	auto line = std::format("{},{}", name, asset);


	if (std::find(prepared_source.begin(), prepared_source.end(), line) == prepared_source.end())
	{
		// No dupes in mah source!
		prepared_source.emplace_back(line);
	}
}

void exporter::add_rename_asset(const std::string& old_name, const std::string& new_name)
{
	rawfile_rename_map[old_name] = new_name;
}

std::string exporter::get_source()
{
	std::stringstream source{};
	source << "\n#\n# Source generated by IW5Xport\n# Louve@XLabs 2023\n#\n";
	for (auto s : exporter::prepared_source)
	{
		source << s << "\n";
	}

	source << "\n\n\n";

	return source.str();
}

void exporter::DB_AddXAsset_Hk(game::native::XAssetType type, game::native::XAssetHeader* header)
{
	if (reinterpret_cast<void*>(-1) == header)
	{
		// ???
		return;
	}

	auto asset = game::native::XAsset{ type, *header };
	std::string asset_name = game::native::DB_GetXAssetName(&asset);

	if (utils::flags::has_flag("dump_localizedstrings") && type == game::native::XAssetType::ASSET_TYPE_LOCALIZE_ENTRY)
	{
		std::ofstream output("iw5mp.str", std::ios_base::binary | std::ios_base::app);
		auto a = header->localize;

		output << "REFERENCE			" << a->name << "\n" << "LANG_ENGLISH		" << a->value << "\n\n";
	}

	if (capture)
	{
		switch (type)
		{

		case game::native::XAssetType::ASSET_TYPE_SCRIPTFILE:
		{
			if (asset_name[0] == ',') asset_name = asset_name.substr(1);

			auto readable_name = reinterpret_cast<asset_dumpers::iscriptfile*>(asset_dumpers[type])->get_script_name(asset.header.scriptfile);
			if (readable_name.starts_with("common_scripts/_")) break; // Do not capture common scripts & animated models
			if (readable_name.starts_with("maps/animated_models/")) break; // We dump them by parsing precache

			captured_scripts.push_back(asset_name);
			break;
		}

		case game::native::XAssetType::ASSET_TYPE_RAWFILE:
			if (asset_name[0] == ',') asset_name = asset_name.substr(1);
			if (asset_name.ends_with(".atr")) break; // No animtrees, we dump them manually via precache

			captured_rawfiles.push_back(asset_name);
			break;

		case game::native::XAssetType::ASSET_TYPE_XMODEL:
			if (!export_teams_dvar->current.enabled) 
			{
				if (asset_name.contains("body")) break; // Multiplayer team bodies (maybe ultimately we could keep them)
				if (asset_name.contains("viewhands")) break; // Multiplayer viewhands
				if (asset_name.contains("head_")) break; // Multiplayer heads
			}

			if (asset_name[0] == ',') asset_name = asset_name.substr(1);

			// Destructibles
			captured_models.push_back(asset_name);
			break;

		case game::native::XAssetType::ASSET_TYPE_SOUND:
			// Destructibles
			if (asset_name[0] == ',') asset_name = asset_name.substr(1);

			if (asset_name.starts_with("weap_")) break; // Weapons

			captured_snd.push_back(asset_name);
			break;

		case game::native::XAssetType::ASSET_TYPE_FX:
			// Destructibles
			if (asset_name[0] == ',') asset_name = asset_name.substr(1);
			captured_fx.push_back(asset_name);
			break;
		}
	}
}

void __declspec(naked) exporter::DB_AddXAsset_stub()
{
	__asm
	{
		pushad

		push ecx
		push[esp + 0x20 + 8]

		call exporter::DB_AddXAsset_Hk

		add esp, 8

		popad

		// original code
		sub     esp, 10h
		mov     eax, [esp + 18h]

		// go back
		push 0x4CAC57
		retn
	}
}

void Sys_Error_Hk(LPCSTR str)
{
	std::string err = str;

	MessageBoxA(0, str, str, 0);

	__debugbreak();
}

int exporter::SND_SetDataHook(game::native::MssSound*, char*)
{
	game::native::LoadedSound*** loadedSoundPtr = reinterpret_cast<game::native::LoadedSound***>(0x013E2748);
	auto loadedSound = *(*(loadedSoundPtr));

	// We do not dump rightaway, we'll do so when we need to because of soundaliases
	asset_dumpers::iloadedsound::duplicate_sound_data(loadedSound);
	return 0;
}

void* exporter::find_asset_for_api(int iw4_type, const std::string& name)
{
	auto iw5_type = iw4_to_iw5_type_table[static_cast<iw4::native::XAssetType>(iw4_type)];
	game::native::XAssetHeader header{};

	if (iw4_type == iw4::native::ASSET_TYPE_RAWFILE)
	{
		if (name.ends_with(".gsc"))
		{
			auto dumper = reinterpret_cast<asset_dumpers::iscriptfile*>(asset_dumpers[game::native::ASSET_TYPE_SCRIPTFILE]);
			if (dumper)
			{
				auto script_name = name.substr(0, name.size() - 4);
				script_name = dumper->get_obfuscated_string(script_name);
				auto script = game::native::DB_FindXAssetHeader(game::native::ASSET_TYPE_SCRIPTFILE, script_name.data(), 0);
				header = { script };
				iw5_type = game::native::ASSET_TYPE_SCRIPTFILE;
			}
		}
		else
		{
			auto previous_name = name;

			for (const auto& kv : rawfile_rename_map)
			{
				if (kv.second == name)
				{
					previous_name = kv.first;
					break;
				}
			}

			header = game::native::DB_FindXAssetHeader(iw5_type, previous_name.data(), 0);
		}
	}
	else
	{
		header = game::native::DB_FindXAssetHeader(iw5_type, name.data(), 0);
	}

	if (header.data)
	{
		const char* backup_name{};
		std::string new_name;

		if (iw4_type == iw4::native::ASSET_TYPE_RAWFILE)
		{
			std::string original_name = header.rawfile->name;
			if (rawfile_rename_map.contains(original_name))
			{
				new_name = rawfile_rename_map[original_name];
				backup_name = header.rawfile->name;
				header.rawfile->name = new_name.data();
			}
		}

		auto result = convert(iw5_type, header).data;

		if (iw5_type == game::native::ASSET_TYPE_SCRIPTFILE)
		{
			iw5_type = game::native::ASSET_TYPE_RAWFILE;
		}

		add_to_source(iw5_type, name);

		if (backup_name)
		{
			// Restore
			header.rawfile->name = backup_name;
		}

		return result;
	}

	return reinterpret_cast<void*>(0);
}

void exporter::post_load()
{
	if (utils::flags::has_flag("exporter"))
	{
		// Keep sounds around
		utils::hook(0x4B94EC, exporter::SND_SetDataHook, HOOK_CALL).install()->quick();

		// OnFindAsset
		utils::hook(0x4CAC50, exporter::DB_AddXAsset_stub, HOOK_JUMP).install()->quick();

		// Hide splash window
		utils::hook::nop(0x5CCE28, 5);

		//// Return on Com_StartHunkUsers
		utils::hook::set<unsigned char>(0x556380, 0xC3);

		//// Return on CL_InitRenderer
		utils::hook::set<unsigned char>(0x48EEA0, 0xC3);

		// Return on com_frame_try_block
		utils::hook::set<unsigned char>(0x556470, 0xC3);

		// Kill command whitelist
		utils::hook::set<unsigned char>(0X00553BA0, 0xC3);

		// catch sys_error
		utils::hook(0x5CC43E, Sys_Error_Hk, HOOK_CALL).install()->quick();

		utils::hook(0x5CCED8, event_loop, HOOK_CALL).install()->quick();
		utils::hook(0x5CCE4E, &perform_common_initialization, HOOK_CALL).install()->quick();

		iw4of::params_t params;

		params.write_only_once = true;

		params.fs_read_file = [](const std::string& filename)
		{
			return game::native::filesystem_read_big_file(filename.data(), game::native::FsThread::FS_THREAD_DATABASE);
		};

		params.get_from_string_table = [](unsigned int index)
		{
			return game::native::SL_ConvertToString(index);
		};

		params.print = [](int level, const std::string& message)
		{
			if (level)
			{
				console::error(message.data());
				assert(false);
			}
			else
			{
				console::info(message.data());
			}
		};

		params.find_other_asset = find_asset_for_api;

		params.work_directory = "iw5xport_out/default";


		iw4of_api = new iw4of::api(params);

		scheduler::once([&params]() {
			console::info("ready!\n");

			ready = true;

			}, scheduler::pipeline::main);
	}
}


REGISTER_MODULE(exporter)
