package app

import io.ktor.http.*
import io.ktor.server.application.*
import io.ktor.server.plugins.origin
import io.ktor.server.request.*
import io.ktor.server.routing.*
import io.ktor.server.websocket.*
import io.ktor.websocket.*
import kotlinx.serialization.json.*
import org.slf4j.LoggerFactory
import java.util.concurrent.atomic.AtomicInteger

private val json = Json { ignoreUnknownKeys = true; isLenient = true }
private val rpcLog = LoggerFactory.getLogger("rpc")

fun Application.installWebSocketRpcFor(service: Service, registry: RpcRegistry) {
    routing {
        webSocket(path = "/v1/rpc", protocol = "pragma") {
            val gateway = call.request.queryParameters["gateway"]
            val remote = call.request.origin.remoteAddress
            val ctx = RpcContext(service = service, gateway = gateway)

            rpcLog.info("ws open svc={} gw={} remote={}", service, gateway, remote)

            val seq = AtomicInteger(0)
            suspend fun respondFor(requestId: Int, responseType: String, payloadJson: String) {
                val s = seq.getAndIncrement()
                val msg = "{\"sequenceNumber\":$s,\"response\":{\"requestId\":$requestId,\"type\":\"$responseType\",\"payload\":$payloadJson}}"
                rpcLog.info(
                    "ws out  svc={} gw={} seq={} reqId={} respType={} bytes={}",
                    service, gateway, s, requestId, responseType, msg.length
                )
                outgoing.send(Frame.Text(msg))
            }

            try {
                for (frame in incoming) {
                    when (frame) {
                        is Frame.Text -> {
                            val raw = frame.readText()
                            val obj = runCatching { json.parseToJsonElement(raw).jsonObject }.getOrElse {
                                rpcLog.warn("ws parse-fail svc={} gw={} remote={} raw={}", service, gateway, remote, clip(raw))
                                continue
                            }
                            val requestId = obj["requestId"]?.jsonPrimitive?.intOrNull
                            val requestType = obj["type"]?.jsonPrimitive?.content
                            val payloadElem = obj["payload"] ?: JsonNull

                            if (requestId == null || requestType == null) {
                                rpcLog.warn("ws missing-fields svc={} gw={} raw={}", service, gateway, clip(raw))
                                continue
                            }

                            rpcLog.info(
                                "ws in   svc={} gw={} reqId={} type={} payload={}",
                                service, gateway, requestId, requestType, clip(payloadElem.toString())
                            )

                            val spec = registry.find(service, requestType, payloadElem, ctx)
                            if (spec == null) {
                                rpcLog.warn("ws unhandled svc={} gw={} type={} reqId={}", service, gateway, requestType, requestId)
                                continue
                            }
                            val payload = spec.payloadSupplier(ctx)
                            respondFor(requestId, spec.responseType, payload)
                        }
                        is Frame.Close -> {
                            rpcLog.info("ws close req svc={} gw={} remote={} reason={} code={}",
                                service, gateway, remote, frame.readReason()?.message, frame.readReason()?.code)
                            break
                        }
                        else -> {
                            rpcLog.debug("ws frame svc={} gw={} kind={}", service, gateway, frame.frameType)
                        }
                    }
                }
            } catch (t: Throwable) {
                rpcLog.warn("ws error svc={} gw={} remote={} msg={}", service, gateway, remote, t.message)
                throw t
            } finally {
                rpcLog.info("ws done  svc={} gw={} remote={}", service, gateway, remote)
            }
        }
    }
}

private fun clip(s: String, max: Int = 4096): String =
    if (s.length <= max) s else s.take(max) + "…"