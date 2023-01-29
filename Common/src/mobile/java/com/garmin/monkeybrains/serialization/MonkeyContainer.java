package com.garmin.monkeybrains.serialization;

import java.util.List;

/* loaded from: classes.jar:com/garmin/monkeybrains/serialization/MonkeyContainer.class */
public interface MonkeyContainer {
    List<MonkeyType<?>> getChildren();

    int getChildCount();
}
