package app

import io.ktor.http.*

data class FixedResponse(
    val status: HttpStatusCode,
    val contentType: ContentType,
    val body: ByteArray
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as FixedResponse

        if (status != other.status) return false
        if (contentType != other.contentType) return false
        if (!body.contentEquals(other.body)) return false

        return true
    }

    override fun hashCode(): Int {
        var result = status.hashCode()
        result = 31 * result + contentType.hashCode()
        result = 31 * result + body.contentHashCode()
        return result
    }
}

object FixedRegistry {
    private val map = mutableMapOf<Triple<Service, HttpMethod, String>, FixedResponse>()

    fun load(): FixedRegistry {
        fun bytes(path: String): ByteArray =
            FixedRegistry::class.java.classLoader.getResource(path)
                ?.readBytes() ?: error("Missing payload resource: $path")

        // GAME
        put(Service.GAME, HttpMethod.Get, "/v1/info",
            FixedResponse(HttpStatusCode.OK, ContentType.Application.Json.withCharset(Charsets.UTF_8),
                bytes("payloads/game/v1/info.json")))
        put(Service.GAME, HttpMethod.Get, "/v1/spectre/healthcheck-status",
            FixedResponse(HttpStatusCode.OK, ContentType.Text.Plain.withCharset(Charsets.UTF_8),
                bytes("payloads/game/v1/spectre/healthcheck-status.txt")))
        put(Service.GAME, HttpMethod.Get, "/v1/types",
            FixedResponse(HttpStatusCode.OK, ContentType.Application.Json.withCharset(Charsets.UTF_8),
                bytes("payloads/game/v1/types.json")))
        put(Service.GAME, HttpMethod.Get, "/v1/loginqueue/getinqueuev1",
            FixedResponse(HttpStatusCode.OK, ContentType.Application.Json.withCharset(Charsets.UTF_8),
                bytes("payloads/game/v1/loginqueue/getinqueuev1.json")))
        put(Service.GAME, HttpMethod.Get, "/v1/gateway",
            FixedResponse(HttpStatusCode.OK, ContentType.Application.Json.withCharset(Charsets.UTF_8),
                bytes("payloads/game/v1/gateway.json")))

        // SOCIAL
        put(Service.SOCIAL, HttpMethod.Get, "/v1/healthcheck",
            FixedResponse(HttpStatusCode.OK, ContentType.Application.Json.withCharset(Charsets.UTF_8),
                bytes("payloads/social/v1/healthcheck.json")))
        put(Service.SOCIAL, HttpMethod.Post, "/v1/account/authenticateorcreatev2",
            FixedResponse(HttpStatusCode.OK, ContentType.Application.Json.withCharset(Charsets.UTF_8),
                bytes("payloads/social/v1/account/authenticateorcreatev2.json")))
        put(Service.SOCIAL, HttpMethod.Get, "/v1/gateway",
            FixedResponse(HttpStatusCode.OK, ContentType.Application.Json.withCharset(Charsets.UTF_8),
                bytes("payloads/social/v1/gateway.json")))

        return this
    }

    private fun put(service: Service, method: HttpMethod, path: String, resp: FixedResponse) {
        map[Triple(service, method, path)] = resp
    }

    fun find(service: Service, method: HttpMethod, path: String): FixedResponse? =
        map[Triple(service, method, path)]
}