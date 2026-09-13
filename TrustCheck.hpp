#pragma once

#include <windows.h>
#include <wincrypt.h>
#include <wintrust.h>

#include <cstddef>
#include <cstdint>

bool TrustCheck(
	const uint8_t* trustedRootDer,
	size_t trustedRootDerSize,
	bool validateCertificateChainRoot = true);

int SafeCheck(
	const uint8_t* trustedRootDer,
	size_t trustedRootDerSize,
	bool validateCertificateChainRoot = true);

