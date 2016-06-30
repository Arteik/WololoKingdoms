#include "ai900unitidfix.h"
#include "wololo/fix.h"
#include "genie/dat/DatFile.h"

namespace wololo {

/*
 * In AOC, AIs can't properly interact with Units that have an ID over 899
 * This algorithm is a workaround that works by swapping the units and
 * searching the whole dat file for references to their IDs, changing them accordingly,
 * essentially doing in a second what would take hours of manual work
*/

std::vector<std::pair<int, int>> const unitsIDtoSwap = {
	{1001, 106}, // Organ Gun, INFIL_D
	{1003, 114}, // Elite Organ Gun, LNGBT_D
	{1004, 162}, // Caravel, FLAGX
	{1006, 183}, // Elite Caravel, TMISB
	{1007, 203}, // Camel Archer, VDML
	{1009, 208}, // Elite Camel Archer, TWAL
	{1010, 223}, // Genitour, VFREP_D
	{1012, 230}, // Elite Genitour, VMREP_D
	{1013, 260}, // Gbeto, OLD-FISH3
	{1015, 418}, // Elite Gbeto, TROCK
	{1016, 453}, // Shotel Warrior, DOLPH4
	{1018, 459}, // Elite Shotel Warrior, FISH5
	{1079, 732}, // Genitour placeholder, HKHAN_D
	{1103, 703}, // Fire Galley, HKUSH_D
	{1104, 527} // Demolition Raft, Demolition Ship (This one is special, and allows the Demolition raft to actually self-destruct since this attribute is hardcoded based on the id) // TODO test if the blast damage distance falloff actually works and notifiy AI creators about the change
};


void swapId(int32_t *val, int32_t id1, int32_t id2) {
	if (*val == id1) {
		*val = id2;
	}
	else if (*val == id2) {
		*val = id1;
	}
}

void swapId(int16_t *val, int16_t id1, int16_t id2) {
	if (*val == id1) {
		*val = id2;
	}
	else if (*val == id2) {
		*val = id1;
	}
}

void swapIdInCommon(genie::techtree::Common *common, int id1, int id2) {
	for (int i = 0; i < common->SlotsUsed; i++) {
		if (common->Mode[i] == 2) { // Unit
			swapId(&common->UnitResearch[i], id1, id2);
		}
	}
}

void swapUnits(genie::DatFile *aocDat, int id1, int id2) {
	// First : swap the actual units
	genie::UnitHeader tmpHeader = aocDat->UnitHeaders[id1];
	aocDat->UnitHeaders[id1] = aocDat->UnitHeaders[id2];
	aocDat->UnitHeaders[id2] = tmpHeader;
	for (size_t civIndex = 0; civIndex < aocDat->Civs.size(); ++civIndex) {
		aocDat->Civs[civIndex].Units[id1].ID1 = id2;
		aocDat->Civs[civIndex].Units[id2].ID1 = id1;
		genie::Unit tmpUnit = aocDat->Civs[civIndex].Units[id1];
		aocDat->Civs[civIndex].Units[id1] = aocDat->Civs[civIndex].Units[id2];
		aocDat->Civs[civIndex].Units[id2] = tmpUnit;
		uint32_t tmpPointer = aocDat->Civs[civIndex].UnitPointers[id1];
		aocDat->Civs[civIndex].UnitPointers[id1] = aocDat->Civs[civIndex].UnitPointers[id2];
		aocDat->Civs[civIndex].UnitPointers[id2] = tmpPointer;
	}


	// Then : modify all references to these units

	// Iterate techs
	for (std::vector<genie::Techage>::iterator techIt = aocDat->Techages.begin(), end = aocDat->Techages.end(); techIt != end; ++techIt) {
		// Iterate effects of each tech
		for (std::vector<genie::TechageEffect>::iterator techEffectsIt = techIt->Effects.begin(), end = techIt->Effects.end(); techEffectsIt != end; ++techEffectsIt) {
			switch (techEffectsIt->Type) {
			case 3: // upgrade unit (this ones uses 2 units hence the special case, notice the absence of break)
				swapId(&techEffectsIt->B, id1, id2);
			case 0: // attribute modifier
			case 2: // enable/disable unit
			case 4: // attribute modifier (+/-)
			case 5: // attribute modifier (*)
				swapId(&techEffectsIt->A, id1, id2);
			}
		}
	}

	// Iterate tech tree ages
	for (std::vector<genie::TechTreeAge>::iterator ageIt = aocDat->TechTree.TechTreeAges.begin(), end = aocDat->TechTree.TechTreeAges.end(); ageIt != end; ++ageIt) {
		// Iterate connected units of each age
		for (std::vector<int32_t>::iterator unitIt = ageIt->Units.begin(), end = ageIt->Units.end(); unitIt != end; ++unitIt) {
			swapId(&(*unitIt), id1, id2);
		}
	}

	// Iterate tech tree buildings
	for (std::vector<genie::BuildingConnection>::iterator buildingIt = aocDat->TechTree.BuildingConnections.begin(), end = aocDat->TechTree.BuildingConnections.end(); buildingIt != end; buildingIt++) {
		// Iterate connected units of each age
		for (std::vector<int32_t>::iterator unitIt = buildingIt->Units.begin(), end = buildingIt->Units.end(); unitIt != end; ++unitIt) {
			swapId(&(*unitIt), id1, id2);
		}
		swapIdInCommon(&buildingIt->Common, id1, id2);
	}

	// Iterate tech tree units
	for (std::vector<genie::UnitConnection>::iterator unitIt = aocDat->TechTree.UnitConnections.begin(), end = aocDat->TechTree.UnitConnections.end(); unitIt != end; ++unitIt) {
		swapId(&unitIt->ID, id1, id2);
		// Iterate connected units of each unit
		for (std::vector<int32_t>::iterator unitUnitIt = unitIt->Units.begin(), end = unitIt->Units.end(); unitUnitIt != end; ++unitUnitIt) {
			swapId(&(*unitUnitIt), id1, id2);
		}
		swapIdInCommon(&unitIt->Common, id1, id2);
	}

	// Iterate tech tree researches
	for (std::vector<genie::ResearchConnection>::iterator researchIt = aocDat->TechTree.ResearchConnections.begin(), end = aocDat->TechTree.ResearchConnections.end(); researchIt != end; researchIt++) {
		// Iterate connected units of each researches
		for (std::vector<int32_t>::iterator researchUnitIt = researchIt->Units.begin(), end = researchIt->Units.end(); researchUnitIt != end; ++researchUnitIt) {
			swapId(&(*researchUnitIt), id1, id2);
		}
		swapIdInCommon(&researchIt->Common, id1, id2);
	}

	// Iterate through Civs Units to replace the dead unit graphic if necessary
	for (std::vector<genie::Civ>::iterator civIt = aocDat->Civs.begin(), end = aocDat->Civs.end(); civIt != end; ++civIt) {
		for (std::vector<genie::Unit>::iterator unitIt = civIt->Units.begin(), end = civIt->Units.end(); unitIt != end; ++unitIt) {
			swapId(&unitIt->DeadUnitID, id1, id2);
		}
	}
}

void ai900unitidPatch(genie::DatFile *aocDat, std::map<int, std::string> *langReplacement) {
	for (size_t i = 0; i < unitsIDtoSwap.size(); i++) {
		swapUnits(aocDat, unitsIDtoSwap[i].first, unitsIDtoSwap[i].second);
	}
}

Fix ai900UnitIdFix = {
	&ai900unitidPatch,
	"AI can't use unit id over 900 workaround"
};

}

