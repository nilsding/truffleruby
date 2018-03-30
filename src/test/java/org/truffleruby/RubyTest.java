/*
 * Copyright (c) 2016, 2018 Oracle and/or its affiliates. All rights reserved. This
 * code is released under a tri EPL/GPL/LGPL license. You can use it,
 * redistribute it and/or modify it under the terms of the:
 *
 * Eclipse Public License version 1.0
 * GNU General Public License version 2
 * GNU Lesser General Public License version 2.1
 */
package org.truffleruby;

import com.oracle.truffle.api.interop.TruffleObject;
import com.oracle.truffle.api.interop.java.JavaInterop;
import com.oracle.truffle.api.nodes.Node;
import com.oracle.truffle.api.nodes.NodeUtil;

import org.graalvm.polyglot.Context;
import org.graalvm.polyglot.Source;
import org.jcodings.specific.UTF8Encoding;
import org.truffleruby.language.RubyRootNode;
import org.truffleruby.launcher.RubyLauncher;
import org.truffleruby.launcher.options.OptionsCatalog;
import org.truffleruby.parser.ParserContext;
import org.truffleruby.parser.TranslatorDriver;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.util.List;
import java.util.function.Consumer;

import static org.junit.Assert.assertEquals;

public abstract class RubyTest {

    /** Load a resource as a Source of the new Context API. */
    public static Source getSource(String path) {
        final InputStream stream = ClassLoader.getSystemResourceAsStream(path);
        final Reader reader = new InputStreamReader(stream);
        try {
            return Source.newBuilder(RubyLauncher.LANGUAGE_ID, reader, new File(path).getName()).build();
        } catch (IOException e) {
            throw new Error(e);
        }
    }

    protected <T extends Node> void testWithNode(String text, Class<T> nodeClass, Consumer<T> test) {
        testWithAST(text, (root) -> {
            final List<T> instances = NodeUtil.findAllNodeInstances(root, nodeClass);
            assertEquals(1, instances.size());
            final T node = instances.get(0);
            test.accept(node);
        });
    }

    protected void testWithAST(String text, Consumer<RubyRootNode> test) {
        final com.oracle.truffle.api.source.Source source =
                com.oracle.truffle.api.source.Source.newBuilder(text).name("test.rb").mimeType(RubyLanguage.MIME_TYPE).build();

        testInContext(() -> {
            final TranslatorDriver translator = new TranslatorDriver(RubyLanguage.getCurrentContext());
            final RubyRootNode rootNode = translator.parse(source, UTF8Encoding.INSTANCE, ParserContext.TOP_LEVEL, null, null, null, true, null);
            rootNode.adoptChildren();
            test.accept(rootNode);
        });
    }

    protected void testInContext(Runnable test) {
        final TruffleObject testTruffleObject = JavaInterop.asTruffleFunction(Runnable.class, test);

        try (Context context = setupContext(Context.newBuilder()).build()) {
            context.eval(Source.create(RubyLauncher.LANGUAGE_ID, "-> test { test.call }"))
                    .execute(testTruffleObject);
        }
    }

    public static Context.Builder setupContext(Context.Builder builder) {
        final String cwd = System.getProperty("user.dir");

        return builder
                .option(OptionsCatalog.EXCEPTIONS_TRANSLATE_ASSERT.getName(), Boolean.FALSE.toString())
                .option(OptionsCatalog.HOME.getName(), cwd)
                .option(OptionsCatalog.BASICOPS_INLINE.getName(), Boolean.FALSE.toString())
                .option(OptionsCatalog.GRAAL_WARNING_UNLESS.getName(), Boolean.FALSE.toString());
    }

}
