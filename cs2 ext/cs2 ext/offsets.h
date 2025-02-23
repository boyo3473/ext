#include <cstddef>
#include "getoffsets.h"
namespace offsets {
	uintptr_t dwEntityList = OffsetFetcher::GetOffsetFromOffsets(13);
	uintptr_t dwLocalPlayerPawn = OffsetFetcher::GetOffsetFromOffsets(20);
	uintptr_t dwLocalPlayerController = OffsetFetcher::GetOffsetFromOffsets(19);
	uintptr_t dwViewAngles = OffsetFetcher::GetOffsetFromOffsets(25);
	uintptr_t dwViewMatrix = OffsetFetcher::GetOffsetFromOffsets(26);

	uintptr_t m_iIDEntIndex = OffsetFetcher::GetOffsetFromClient(2152);
	uintptr_t m_iTeamNum = OffsetFetcher::GetOffsetFromClient(5848);
	uintptr_t m_iHealth = OffsetFetcher::GetOffsetFromClient(5815);
	uintptr_t m_vOldOrigin = OffsetFetcher::GetOffsetFromClient(2722);
	uintptr_t m_pClippingWeapon = OffsetFetcher::GetOffsetFromClient(2116);
	uintptr_t m_vecViewOffset = OffsetFetcher::GetOffsetFromClient(476);
	uintptr_t m_pGameSceneNode = OffsetFetcher::GetOffsetFromClient(5811);
	uintptr_t m_modelState = OffsetFetcher::GetOffsetFromClient(938);
	uintptr_t m_hPlayerPawn = OffsetFetcher::GetOffsetFromClient(6323);
	uintptr_t m_sSanitizedPlayerName = OffsetFetcher::GetOffsetFromClient(6294);
	uintptr_t m_szName = OffsetFetcher::GetOffsetFromClient(6111);

	uintptr_t m_vecAbsOrigin = OffsetFetcher::GetOffsetFromClient(6580);


}
