/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_trafodion_sql_HDFSClient */

#ifndef _Included_org_trafodion_sql_HDFSClient
#define _Included_org_trafodion_sql_HDFSClient
#ifdef __cplusplus
extern "C" {
#endif
#undef org_trafodion_sql_HDFSClient_UNCOMPRESSED
#define org_trafodion_sql_HDFSClient_UNCOMPRESSED 1L
/*
 * Class:     org_trafodion_sql_HDFSClient
 * Method:    copyToByteBuffer
 * Signature: (Ljava/nio/ByteBuffer;I[BI)I
 */
JNIEXPORT jint JNICALL Java_org_trafodion_sql_HDFSClient_copyToByteBuffer
  (JNIEnv *, jobject, jobject, jint, jbyteArray, jint);

/*
 * Class:     org_trafodion_sql_HDFSClient
 * Method:    sendFileStatus
 * Signature: (JIIZLjava/lang/String;JJSJLjava/lang/String;Ljava/lang/String;SJ)I
 */
JNIEXPORT jint JNICALL Java_org_trafodion_sql_HDFSClient_sendFileStatus
  (JNIEnv *, jobject, jlong, jint, jint, jboolean, jstring, jlong, jlong, jshort, jlong, jstring, jstring, jshort, jlong);

#ifdef __cplusplus
}
#endif
#endif
