package com.jiaquan.myplayer.util;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Annotation used to mark methods that are called from native code.
 */
@Target({ElementType.CONSTRUCTOR, ElementType.METHOD})
@Retention(RetentionPolicy.CLASS)
public @interface CalledByNative {
    /*
     *  If present, tells which inner class the method belongs to.
     */
    public String value() default "";
}
