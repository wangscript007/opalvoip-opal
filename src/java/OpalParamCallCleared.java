/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.35
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package -I/home/robertj/opal/include;

public class OpalParamCallCleared {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected OpalParamCallCleared(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(OpalParamCallCleared obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      exampleJNI.delete_OpalParamCallCleared(swigCPtr);
    }
    swigCPtr = 0;
  }

  public void setM_callToken(String value) {
    exampleJNI.OpalParamCallCleared_m_callToken_set(swigCPtr, this, value);
  }

  public String getM_callToken() {
    return exampleJNI.OpalParamCallCleared_m_callToken_get(swigCPtr, this);
  }

  public void setM_reason(OpalCallEndReason value) {
    exampleJNI.OpalParamCallCleared_m_reason_set(swigCPtr, this, value.swigValue());
  }

  public OpalCallEndReason getM_reason() {
    return OpalCallEndReason.swigToEnum(exampleJNI.OpalParamCallCleared_m_reason_get(swigCPtr, this));
  }

  public OpalParamCallCleared() {
    this(exampleJNI.new_OpalParamCallCleared(), true);
  }

}
