/*
 * Copyright (c) 2015, 2017 Oracle and/or its affiliates. All rights reserved. This
 * code is released under a tri EPL/GPL/LGPL license. You can use it,
 * redistribute it and/or modify it under the terms of the:
 *
 * Eclipse Public License version 1.0
 * GNU General Public License version 2
 * GNU Lesser General Public License version 2.1
 */
package org.truffleruby.core.fiber;

import com.oracle.truffle.api.object.DynamicObject;
import com.oracle.truffle.api.object.DynamicObjectFactory;
import com.oracle.truffle.api.object.dsl.Layout;
import com.oracle.truffle.api.object.dsl.Volatile;
import org.truffleruby.core.basicobject.BasicObjectLayout;

import java.util.concurrent.BlockingQueue;

@Layout
public interface FiberLayout extends BasicObjectLayout {

    DynamicObjectFactory createFiberShape(DynamicObject logicalClass,
                                          DynamicObject metaClass);

    DynamicObject createFiber(DynamicObjectFactory factory,
                              FiberData fiberData,
                              BlockingQueue<FiberManager.FiberMessage> messageQueue,
                              DynamicObject rubyThread,
                              @Volatile boolean transferred);

    boolean isFiber(DynamicObject object);

    FiberData getFiberData(DynamicObject object);

    BlockingQueue<FiberManager.FiberMessage> getMessageQueue(DynamicObject object);

    DynamicObject getRubyThread(DynamicObject object);

    boolean getTransferred(DynamicObject object);
    void setTransferred(DynamicObject object, boolean value);

}
