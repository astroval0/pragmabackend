package app

import io.ktor.http.*
import io.ktor.http.content.*
import io.ktor.server.application.*
import io.ktor.server.request.*
import io.ktor.server.response.*
import io.ktor.utils.io.*
import kotlinx.coroutines.awaitCancellation

fun Application.installConnectHandler() {
    intercept(ApplicationCallPipeline.Call) {
        if (call.request.httpMethod.value.equals("CONNECT", ignoreCase = true)) {
            val keepAlive = call.request.headers[HttpHeaders.Connection]?.contains("keep-alive", true) == true
            call.response.headers.append(HttpHeaders.Connection, if (keepAlive) "keep-alive" else "close")
            call.respond(object : OutgoingContent.WriteChannelContent() {
                override val status: HttpStatusCode = HttpStatusCode(200, "Connection Established")
                override suspend fun writeTo(channel: ByteWriteChannel) {
                    // No body. Keep connection open until client closes.
                    try {
                        awaitCancellation()
                    } finally {
                        channel.close()
                    }
                }
            })
            finish()
        }
    }
}