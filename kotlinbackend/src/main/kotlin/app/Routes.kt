package app

import io.ktor.http.*
import io.ktor.server.application.*
import io.ktor.server.request.*
import io.ktor.server.response.*
import io.ktor.server.routing.*

fun Application.installRoutesFor(service: Service, registry: FixedRegistry) {
    intercept(ApplicationCallPipeline.Call) {
        val method = call.request.httpMethod
        val path = call.request.path()

        val fixed = registry.find(service, method, path)
        if (fixed != null) {
            call.response.headers.append(HttpHeaders.ContentType, fixed.contentType.toString())
            call.response.headers.append(HttpHeaders.Vary, "Origin")
            call.respondBytes(fixed.body, fixed.contentType, fixed.status)
            finish(); return@intercept
        }
    }
}