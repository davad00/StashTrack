import React from "react";
import { View } from "react-juce";

const colors = {
  background: "#0A0B0A",
  panel: "#101210",
  panelLift: "#14170F",
  well: "#0C0E0C",
  line: "#2B3029",
  accent: "#C6F135",
};

function Bar({ width, height, left, top }) {
  return (
    <View
      style={{
        width,
        height,
        left,
        top,
        backgroundColor: colors.accent,
        position: "absolute",
      }}
    />
  );
}

export default function App() {
  return (
    <View
      style={{
        width: "100%",
        height: "100%",
        backgroundColor: colors.background,
        padding: 14,
      }}
    >
      <View
        style={{
          flex: 1,
          backgroundColor: colors.panel,
          borderColor: colors.line,
          borderWidth: 1,
        }}
      >
        <View
          style={{
            height: 52,
            backgroundColor: colors.well,
            borderBottomColor: colors.line,
            borderBottomWidth: 1,
          }}
        >
          <Bar width={10} height={14} left={22} top={19} />
          <Bar width={10} height={10} left={38} top={23} />
          <Bar width={10} height={18} left={54} top={15} />
        </View>

        <View
          style={{
            margin: 18,
            marginTop: 64,
            flex: 1,
            backgroundColor: colors.well,
            borderColor: colors.line,
            borderWidth: 1,
          }}
        >
          <View
            style={{
              height: 42,
              flexDirection: "row",
              margin: 18,
              marginBottom: 12,
            }}
          >
            <View
              style={{
                flex: 1,
                backgroundColor: colors.background,
                borderColor: colors.line,
                borderWidth: 1,
                marginRight: 12,
              }}
            />
            <View
              style={{
                width: 124,
                backgroundColor: colors.accent,
              }}
            />
          </View>

          <View
            style={{
              height: 42,
              flexDirection: "row",
              marginLeft: 18,
              marginRight: 18,
              marginBottom: 14,
            }}
          >
            <View
              style={{
                width: 78,
                backgroundColor: colors.panelLift,
                borderColor: colors.line,
                borderWidth: 1,
                marginRight: 12,
              }}
            />
            <View
              style={{
                width: 116,
                backgroundColor: colors.background,
                borderColor: colors.line,
                borderWidth: 1,
                marginRight: 54,
              }}
            />
            <View
              style={{
                width: 116,
                backgroundColor: colors.background,
                borderColor: colors.line,
                borderWidth: 1,
              }}
            />
          </View>

          <View
            style={{
              flex: 1,
              backgroundColor: colors.panelLift,
              borderColor: colors.line,
              borderWidth: 1,
              margin: 18,
              padding: 18,
            }}
          >
            <View
              style={{
                flex: 1,
                backgroundColor: colors.background,
                borderColor: colors.line,
                borderWidth: 1,
              }}
            />
          </View>
        </View>
      </View>
    </View>
  );
}
