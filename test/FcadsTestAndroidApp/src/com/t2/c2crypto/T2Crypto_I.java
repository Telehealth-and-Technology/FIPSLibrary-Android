package com.t2.c2crypto;

public class T2Crypto_I {

	public int initializeLogin(String pinAndAnswersJson) {
		int retVal = 0;
		return retVal;

//		#include <openssl/des.h>
//		#include <openssl/aes.h>
//		#include <openssl/evp.h>		
//        RAND_poll();
//        // Note _salt is malloc'ed in init but key may be different length so we need to do that here
//        int result = RAND_bytes(_salt, SALT_LENGTH);
//        NSAssert(result == OpenSSLSuccess, @"ERROR: - Can't calculate random salt");
//        
//        RIKeyBytes = malloc(sizeof(char) * MAX_KEY_LENGTH);
//        result = RAND_bytes((unsigned char*)RIKeyBytes, MAX_KEY_LENGTH);
		
//	    /*
//	     * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
//	     * nrounds is the number of times the we hash the material. More rounds are more secure but
//	     * slower.
//	     * This uses the KDF algorithm to derrive key from password phrase
//	     */
//	    i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, key_data_len, nrounds, aCredentials->key, aCredentials->iv);
//	    if (i != EVP_aes_256_cbc_Key_LENGTH) {
//	        NSLog(@"ERROR: Key size is %d bits - should be %d bits\n", i, EVP_aes_256_cbc_Key_LENGTH * 8);
//	        return T2Error;
//	    }	
		
		
	}
	
	
	
}
