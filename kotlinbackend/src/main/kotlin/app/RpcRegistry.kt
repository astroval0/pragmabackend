package app

import kotlinx.serialization.json.*

data class RpcResponseSpec(
    val responseType: String,
    val payloadSupplier: (RpcContext) -> String
)

data class RpcContext(val service: Service, val gateway: String? = null)

private data class RpcKey(val service: Service, val requestType: String, val variant: String?)

class RpcRegistry {
    private val responses = mutableMapOf<RpcKey, RpcResponseSpec>()
    private val resolvers = mutableMapOf<Pair<Service, String>, VariantResolver>()

    fun interface VariantResolver { fun resolve(payload: JsonElement, ctx: RpcContext): String? }

    private fun readResource(path: String): String {
        val url = this::class.java.classLoader.getResource(path)
            ?: error("Missing RPC payload resource: $path")
        return String(url.readBytes(), Charsets.UTF_8)
    }

    fun register(
        service: Service,
        requestType: String,
        responseType: String,
        staticPayload: String? = null,
        resourcePath: String? = null,
        variant: String? = null
    ) {
        val supplier: (RpcContext) -> String
        if (staticPayload != null) {
            val payload = staticPayload
            supplier = { _: RpcContext -> payload }
        } else if (resourcePath != null) {
            val body = readResource(resourcePath)
            supplier = { _: RpcContext -> body }
        } else {
            supplier = { _: RpcContext -> "{}" }
        }
        responses[RpcKey(service, requestType, variant)] = RpcResponseSpec(responseType, supplier)
    }

    fun setVariantResolver(service: Service, requestType: String, resolver: VariantResolver) {
        resolvers[service to requestType] = resolver
    }

    fun find(service: Service, requestType: String, payload: JsonElement, ctx: RpcContext): RpcResponseSpec? {
        val variant = resolvers[service to requestType]?.resolve(payload, ctx)
        return responses[RpcKey(service, requestType, variant)]
            ?: responses[RpcKey(service, requestType, null)]
    }

    companion object {
        fun loadDefault(): RpcRegistry = RpcRegistry().apply {
            // Heartbeat for both
            register(Service.GAME,   "PlayerSessionRpc.HeartbeatV1Request", "PlayerSessionRpc.HeartbeatV1Response", staticPayload = "{}")
            register(Service.SOCIAL, "PlayerSessionRpc.HeartbeatV1Request", "PlayerSessionRpc.HeartbeatV1Response", staticPayload = "{}")

            // GAME
            register(Service.GAME, "InventoryRpc.GetInventoryV2Request", "InventoryRpc.GetInventoryV2Response",
                resourcePath = "payloads/ws/game/InventoryRpc.GetInventoryV2Response.json")
            register(Service.GAME, "MtnConfigServiceRpc.GetConfigForClientV1Request", "MtnConfigServiceRpc.GetConfigForClientV1Response",
                resourcePath = "payloads/ws/game/MtnConfigServiceRpc.GetConfigForClientV1Response.json")
            register(Service.GAME, "MtnBeaconServiceRpc.GetBeaconEndpointsV1Request", "MtnBeaconServiceRpc.GetBeaconEndpointsV1Response",
                resourcePath = "payloads/ws/game/beacon/hathora-udp.json", variant = "hathora-udp")
            register(Service.GAME, "MtnBeaconServiceRpc.GetBeaconEndpointsV1Request", "MtnBeaconServiceRpc.GetBeaconEndpointsV1Response",
                resourcePath = "payloads/ws/game/beacon/hathora.json", variant = "hathora")
            setVariantResolver(Service.GAME, "MtnBeaconServiceRpc.GetBeaconEndpointsV1Request",
                VariantResolvers.byField("type"))
            register(Service.GAME, "MtnAnalyticsNodeServiceRpc.RecordClientAnalyticsEventV1Request", "MtnAnalyticsNodeServiceRpc.RecordClientAnalyticsEventV1Response",
                resourcePath = "payloads/ws/game/MtnAnalyticsNodeServiceRpc.RecordClientAnalyticsEventV1Response.json")
            register(Service.GAME, "InventoryRpc.SyncEntitlementsV1Request", "InventoryRpc.SyncEntitlementsV1Response",
                resourcePath = "payloads/ws/game/InventoryRpc.SyncEntitlementsV1Response.json")
            register(Service.GAME, "GameDataRpc.GetLoginDataV3Request", "GameDataRpc.GetLoginDataV3Response",
                resourcePath = "payloads/ws/game/GameDataRpc.GetLoginDataV3Response.json")
            register(Service.GAME, "MtnPlayerDataServiceRpc.GetAllPlayerDataClientV1Request", "MtnPlayerDataServiceRpc.GetAllPlayerDataClientV1Response",
                resourcePath = "payloads/ws/game/MtnPlayerDataServiceRpc.GetAllPlayerDataClientV1Response.json")
            register(Service.GAME, "MtnLoadoutServiceRpc.FetchPlayerOutfitLoadoutsV1Request", "MtnLoadoutServiceRpc.FetchPlayerOutfitLoadoutsV1Response",
                resourcePath = "payloads/ws/game/MtnLoadoutServiceRpc.FetchPlayerOutfitLoadoutsV1Response.json")
            register(Service.GAME, "MtnLoadoutServiceRpc.FetchPlayerWeaponLoadoutsV1Request", "MtnLoadoutServiceRpc.FetchPlayerWeaponLoadoutsV1Response",
                resourcePath = "payloads/ws/game/MtnLoadoutServiceRpc.FetchPlayerWeaponLoadoutsV1Response.json")
            register(Service.GAME, "MultiplayerRpc.InitializePartyV1Request", "MultiplayerRpc.InitializePartyV1Response",
                resourcePath = "payloads/ws/game/MultiplayerRpc.InitializePartyV1Response.json")
            register(Service.GAME, "MultiplayerRpc.SyncPartyV1Request", "MultiplayerRpc.SyncPartyV1Response",
                resourcePath = "payloads/ws/game/MultiplayerRpc.SyncPartyV1Response.json")
            // SOCIAL
            register(Service.SOCIAL, "FriendRpc.SetPresenceV1Request", "FriendRpc.SetPresenceV1Response",
                resourcePath = "payloads/ws/social/FriendRpc.SetPresenceV1Response.json")
            register(Service.SOCIAL, "GetPlayerIdentitiesByProviderAccountIdsV1Request", "GetPlayerIdentitiesByProviderAccountIdsV1Response",
                resourcePath = "payloads/ws/social/GetPlayerIdentitiesByProviderAccountIdsV1Response.json"
            )
        }
    }
}

object VariantResolvers {
    val asString: RpcRegistry.VariantResolver = RpcRegistry.VariantResolver { payload, _ ->
        val p = payload as? JsonPrimitive
        if (p != null && p.isString) p.content else null
    }
    fun byField(fieldName: String): RpcRegistry.VariantResolver = RpcRegistry.VariantResolver { payload, _ ->
        val obj = payload as? JsonObject
        val prim = obj?.get(fieldName) as? JsonPrimitive
        if (prim != null && prim.isString) prim.content else null
    }
}