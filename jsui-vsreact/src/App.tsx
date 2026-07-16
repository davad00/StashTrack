import { useEffect, useState } from "react";
import { View, Text, TextInput, NativeView, native } from "@vsreact/core";

type Tone = "muted" | "accent" | "error";

const toneClass: Record<Tone, string> = {
  muted: "text-muted",
  accent: "text-accent",
  error: "text-error",
};

function LogoMark() {
  return (
    <View className="w-[42] h-full relative">
      <View className="absolute bg-accent w-[10] h-[14] left-0 top-[19]" />
      <View className="absolute bg-accent w-[10] h-[10] left-[16] top-[23]" />
      <View className="absolute bg-accent w-[10] h-[18] left-[32] top-[15]" />
    </View>
  );
}

function Label({ children }: { children: string }) {
  return <Text className="text-muted text-xs font-bold tracking-wide">{children}</Text>;
}

function Checkbox({
  checked,
  disabled,
  label,
  onToggle,
}: {
  checked: boolean;
  disabled: boolean;
  label: string;
  onToggle: () => void;
}) {
  return (
    <View
      className={`flex-row items-center gap-2 cursor-pointer ${disabled ? "opacity-50" : ""}`}
      onClick={disabled ? undefined : onToggle}
    >
      <View
        className={`w-[18] h-[18] rounded border ${
          checked ? "bg-accent border-accent" : "bg-well border-line hover:border-accent"
        } items-center justify-center`}
      >
        {checked ? <Text className="text-background text-xs font-bold">X</Text> : null}
      </View>
      <Text className="text-text text-sm">{label}</Text>
    </View>
  );
}

function Button({
  label,
  disabled,
  onClick,
}: {
  label: string;
  disabled: boolean;
  onClick: () => void;
}) {
  return (
    <View
      className={`w-[124] items-center justify-center rounded-md ${
        disabled
          ? "bg-line opacity-70"
          : "bg-accent cursor-pointer hover:bg-lime-200 active:bg-lime-500"
      }`}
      onClick={disabled ? undefined : onClick}
    >
      <Text className={`${disabled ? "text-muted" : "text-background"} font-bold text-sm tracking-wide`}>
        {label}
      </Text>
    </View>
  );
}

const inputClasses =
  "bg-well border border-line focus:border-accent rounded-md px-3 text-text text-[15]";

export default function App() {
  const [version, setVersion] = useState("");
  const [url, setUrl] = useState("");
  const [clip, setClip] = useState(false);
  const [start, setStart] = useState("");
  const [end, setEnd] = useState("");
  const [downloading, setDownloading] = useState(false);
  const [status, setStatus] = useState<{ message: string; tone: Tone }>({
    message: "",
    tone: "muted",
  });

  useEffect(() => {
    const initial = native.call("getInitialState");
    if (initial) {
      setVersion(String(initial.version ?? ""));
      setStatus({ message: String(initial.choice ?? ""), tone: "muted" });
    }

    const offStatus = native.on("status", (p) =>
      setStatus({ message: String(p?.message ?? ""), tone: (p?.tone as Tone) ?? "muted" }),
    );
    const offState = native.on("downloadState", (p) => setDownloading(Boolean(p?.running)));

    return () => {
      offStatus();
      offState();
    };
  }, []);

  const startDownload = () => {
    if (downloading) return;
    native.call("startDownload", { url: url.trim(), clip, start, end });
  };

  const fieldsDisabled = downloading;
  const clipFieldsDisabled = !clip || downloading;

  return (
    <View className="w-full h-full bg-background p-[14]">
      <View className="flex-1 bg-panel border border-line">
        {/* Header */}
        <View className="h-[52] bg-well flex-row items-center px-[22]">
          <LogoMark />
          <Text className="text-text text-2xl font-bold ml-3">
            {version === "" ? "StashTrack" : `StashTrack v${version}`}
          </Text>
          <View className="flex-1" />
          <Text className={`${toneClass[status.tone]} text-xs text-right`}>{status.message}</Text>
        </View>
        <View className="h-px bg-line" />

        {/* Body */}
        <View className="flex-1 p-[18] gap-3">
          <Label>SOURCE URL</Label>

          <View className="h-[42] flex-row gap-3">
            <TextInput
              className={`flex-1 h-full ${inputClasses} ${fieldsDisabled ? "opacity-50" : ""}`}
              placeholder="https://www.youtube.com/watch?v=..."
              value={url}
              disabled={fieldsDisabled}
              onChange={setUrl}
              onSubmit={startDownload}
            />
            <Button
              label={downloading ? "WORKING..." : "DOWNLOAD"}
              disabled={downloading}
              onClick={startDownload}
            />
          </View>

          <View className="h-[42] flex-row items-center gap-3">
            <Checkbox
              checked={clip}
              disabled={downloading}
              label="Clip"
              onToggle={() => setClip((c) => !c)}
            />
            <Label>START</Label>
            <TextInput
              className={`w-[116] h-full ${inputClasses} ${clipFieldsDisabled ? "opacity-50" : ""}`}
              placeholder="0:30"
              value={start}
              disabled={clipFieldsDisabled}
              onChange={setStart}
            />
            <Label>END</Label>
            <TextInput
              className={`w-[116] h-full ${inputClasses} ${clipFieldsDisabled ? "opacity-50" : ""}`}
              placeholder="1:00"
              value={end}
              disabled={clipFieldsDisabled}
              onChange={setEnd}
            />
          </View>

          {/* Waveform (native component: drag into the DAW stays C++) */}
          <NativeView nativeId="waveform" className="flex-1 mt-1" />
        </View>
      </View>
    </View>
  );
}
