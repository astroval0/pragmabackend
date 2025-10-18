package app

import io.ktor.http.*
import io.ktor.server.application.*
import io.ktor.server.engine.*
import io.ktor.server.netty.*
import io.ktor.server.plugins.callloging.*
import io.ktor.server.plugins.defaultheaders.*
import io.ktor.server.websocket.*
import java.time.Duration
import org.slf4j.event.Level

fun main() {
    val gamePort = System.getenv("GAME_PORT")?.toIntOrNull() ?: 8081
    val socialPort = System.getenv("SOCIAL_PORT")?.toIntOrNull() ?: 8082

    val game = embeddedServer(Netty, host = "127.0.0.1", port = gamePort) {
        install(DefaultHeaders) { header(HttpHeaders.Vary, "Origin") }
        install(CallLogging) { level = Level.INFO }
        install(WebSockets) {
            pingPeriod = Duration.ofSeconds(30); timeout = Duration.ofSeconds(30)
            maxFrameSize = Long.MAX_VALUE; masking = false
            timeout = Duration.ZERO
        }
        installConnectHandler()
        installRoutesFor(Service.GAME, FixedRegistry.load())
        installWebSocketRpcFor(Service.GAME, RpcRegistry.loadDefault())
    }.start(wait = false)

    val social = embeddedServer(Netty, host = "127.0.0.1", port = socialPort) {
        install(DefaultHeaders) { header(HttpHeaders.Vary, "Origin") }
        install(CallLogging) { level = Level.INFO }
        install(WebSockets) {
            pingPeriod = Duration.ofSeconds(30); timeout = Duration.ofSeconds(30)
            maxFrameSize = Long.MAX_VALUE; masking = false
            timeout = Duration.ZERO
        }
        installConnectHandler()
        installRoutesFor(Service.SOCIAL, FixedRegistry.load())
        installWebSocketRpcFor(Service.SOCIAL, RpcRegistry.loadDefault())
    }.start(wait = false)

    Runtime.getRuntime().addShutdownHook(Thread { game.stop(1000, 2000); social.stop(1000, 2000) })
    Thread.currentThread().join()
}