﻿#include <std_include.hpp>

#include "ifx.hpp"
#include "itechniqueset.hpp"
#include "../asset_dumper.hpp"
#include "../exporter.hpp"
#include "module/console.hpp"

#include "utils/io.hpp"
#include "utils/string.hpp"

#include "api.hpp"

#define IW4X_FX_VERSION 2

namespace asset_dumpers
{
	void ifx::convert(const game::native::FxElemVisuals* visuals, iw4::native::FxElemVisuals* into, char elemType)
	{
		switch (elemType)
		{
		case game::native::FX_ELEM_TYPE_MODEL:
		{
			assert(visuals->model);

			if (visuals->model)
			{
				into->model =
					exporter::convert(game::native::XAssetType::ASSET_TYPE_XMODEL, { visuals->model })
					.model;
			}

			break;
		}

		case game::native::FX_ELEM_TYPE_OMNI_LIGHT:
		case game::native::FX_ELEM_TYPE_SPOT_LIGHT:
			// Unused in COD6
			into->anonymous = (void*)0xDEADC0DE;
			break;

		case game::native::FX_ELEM_TYPE_SOUND:
		{

			if (visuals->soundName)
			{
				into->soundName = local_allocator.duplicate_string(visuals->soundName);
			}

			break;
		}

		case game::native::FX_ELEM_TYPE_RUNNER:
		{
			assert(visuals->effectDef.handle);

			if (visuals->effectDef.handle)
			{
				into->effectDef.handle
					= exporter::convert(game::native::XAssetType::ASSET_TYPE_FX, { visuals->effectDef.handle })
					.fx;
			}

			break;
		}

		default:
		{
			if (visuals->material)
			{
				// We reallocate it because we're copying it over to the new one and modifying it soo...
				auto backup = visuals->material->info.name;

				auto suffixed_name = std::string(backup) + itechniqueset::techset_suffix;

				// We have to rename it because it gets otherwise shadowed by iw4!
				visuals->material->info.name = local_allocator.duplicate_string(suffixed_name);

				into->material =
					exporter::convert(game::native::XAssetType::ASSET_TYPE_MATERIAL, { visuals->material })
					.material;

				assert(into->material);

				//// Restore original name
				visuals->material->info.name = backup;
			}

			break;
		}
		}
	}

	void ifx::convert(const game::native::XAssetHeader& header, iw4::native::XAssetHeader& out)
	{
		assert(header.fx);

		auto iw4_fx = local_allocator.allocate<iw4::native::FxEffectDef>();
		const auto native_fx = header.fx;

		iw4_fx->name = native_fx->name;
		iw4_fx->flags = native_fx->flags;
		iw4_fx->totalSize = native_fx->totalSize;
		iw4_fx->msecLoopingLife = native_fx->msecLoopingLife;
		iw4_fx->elemDefCountLooping = native_fx->elemDefCountLooping;
		iw4_fx->elemDefCountOneShot = native_fx->elemDefCountOneShot;
		iw4_fx->elemDefCountEmission = native_fx->elemDefCountEmission;

		auto fx_count = iw4_fx->elemDefCountLooping + iw4_fx->elemDefCountOneShot + iw4_fx->elemDefCountEmission;
		iw4_fx->elemDefs = local_allocator.allocate_array<iw4::native::FxElemDef>(fx_count);

		for (auto i = 0; i < fx_count; i++)
		{
			// They're identical, IW5 just has a seed info that iw4 doesn't have
			static_assert(sizeof(game::native::FxElemDef) > sizeof(iw4::native::FxElemDef));
			memcpy(&iw4_fx->elemDefs[i], &native_fx->elemDefs[i], sizeof iw4::native::FxElemDef);

			auto iw4_def = &iw4_fx->elemDefs[i];
			auto native_def = &native_fx->elemDefs[i];

			// although, some IW5 FX flags will soft-lock IW4. It's very rare (happens on boardwalk) and I'm not sure why
			// For know we can clamp them safely
			iw4_def->flags &= 0x00FFFFFF;

			iw4_def->visuals = iw4::native::FxElemDefVisuals{};

			// fxElemType is IDENTICAL
			if (native_def->elemType == game::native::FX_ELEM_TYPE_DECAL)
			{
				if (native_def->visuals.markArray)
				{
					iw4_def->visuals.markArray = local_allocator.allocate_array<iw4::native::FxElemMarkVisuals>(native_def->visualCount);
					for (char j = 0; j < native_def->visualCount; ++j)
					{
						constexpr auto arr_count = ARRAYSIZE(native_def->visuals.markArray[j].materials);
						static_assert(arr_count == 2);
						for (auto k = 0; k < arr_count; k++)
						{
							auto materials = iw4_def->visuals.markArray[j].materials;

							if (native_def->visuals.markArray[j].materials[k])
							{
								materials[k] =
									exporter::convert(
										game::native::XAssetType::ASSET_TYPE_MATERIAL,
										{ native_def->visuals.markArray[j].materials[k] }
								).material;
							}
						}
					}
				}
			}
			else if (native_def->visualCount > 1)
			{
				if (native_def->visuals.array)
				{
					iw4_def->visuals.array = local_allocator.allocate_array<iw4::native::FxElemVisuals>(native_def->visualCount);

					for (char j = 0; j < native_def->visualCount; ++j)
					{
						convert(&native_def->visuals.array[j], &iw4_def->visuals.array[j], native_def->elemType);
					}
				}
			}
			else if (native_def->visualCount == 1)
			{
				iw4_def->visuals.instance.anonymous = nullptr; // Making sure it's dead
				convert(&native_def->visuals.instance, &iw4_def->visuals.instance, native_def->elemType);
				assert(iw4_def->visuals.instance.anonymous);
			}

			if (native_def->effectOnImpact.handle)
			{
				iw4_def->effectOnImpact.handle = exporter::convert(game::native::ASSET_TYPE_FX, { native_def->effectOnImpact.handle }).fx;
			}

			if (native_def->effectOnDeath.handle)
			{
				iw4_def->effectOnDeath.handle = exporter::convert(game::native::ASSET_TYPE_FX, { native_def->effectOnDeath.handle }).fx;
			}

			if (native_def->effectEmitted.handle)
			{
				iw4_def->effectEmitted.handle = exporter::convert(game::native::ASSET_TYPE_FX, { native_def->effectEmitted.handle }).fx;
			}

			if (native_def->elemType == game::native::FX_ELEM_TYPE_TRAIL)
			{
				// good
			}
			else if (native_def->elemType == game::native::FX_ELEM_TYPE_SPARKFOUNTAIN)
			{
				// good
			}
			else
			{
				// Unsupported
				iw4_def->extended.unknownDef = nullptr;

				// Only case where this is valid
				assert((native_def->extended.unknownDef != nullptr) == (native_def->elemType == game::native::FX_ELEM_TYPE_SPOT_LIGHT));
			}
		}

		out.fx = iw4_fx;
	}

	void ifx::write(const iw4::native::XAssetHeader& header)
	{
		exporter::get_api()->write(iw4::native::ASSET_TYPE_FX, header.data);
	}

	ifx::ifx()
	{
		command::add("dumpFX", [&](const command::params& params)
			{
				if (params.size() < 2) return;

				auto name = params[1];

				if (name == "*"s)
				{
					std::vector<game::native::XAssetHeader> headers{};

					game::native::DB_EnumXAssets(game::native::XAssetType::ASSET_TYPE_FX, [](game::native::XAssetHeader header, void* data) {
						auto headers = reinterpret_cast<std::vector<game::native::XAssetHeader>*>(data);

						headers->push_back(header);

						}, &headers, false);

					for (auto header : headers)
					{
						convert_and_write(header, true);
					}
				}
				else
				{
					auto header = game::native::DB_FindXAssetHeader(game::native::XAssetType::ASSET_TYPE_FX, name, false);

					if (header.data)
					{
						if (!is_already_dumped(header))
						{
							exporter::add_to_source(game::native::XAssetType::ASSET_TYPE_FX, name);
						}

						convert_and_write(header);
					}
					else
					{
						console::info("could not dump fx %s from the database (cannot find it)\n", name);
					}
				}
			});

	}
}