import { useEffect, useState } from "react";
import { View, Text, TextInput, NativeView, native } from "@vsreact/core";

type Tone = "muted" | "accent" | "error";

const toneText: Record<Tone, string> = {
  muted: "text-muted",
  accent: "text-accent",
  error: "text-error",
};

const toneDot: Record<Tone, string> = {
  muted: "bg-muted",
  accent: "bg-accent",
  error: "bg-error",
};

/** Repaint-driving counter for the small animations (logo, dots, progress). */
function useTicker(intervalMs: number, running: boolean): number {
  const [tick, setTick] = useState(0);

  useEffect(() => {
    if (!running) return;
    const id = setInterval(() => setTick((t) => t + 1), intervalMs);
    return () => clearInterval(id);
  }, [intervalMs, running]);

  return tick;
}

/** Animated three-bar equalizer mark — faster and brighter while working. */
function EqLogo({ working }: { working: boolean }) {
  const tick = useTicker(working ? 90 : 320, true);

  const patterns = [
    [8, 14, 10],
    [12, 8, 15],
    [15, 12, 8],
    [10, 16, 12],
    [14, 9, 14],
    [9, 13, 16],
  ];
  const heights = patterns[tick % patterns.length];

  return (
    <View className="w-[38] h-[38] rounded-lg bg-accent/10 border border-accent/25 flex-row items-end justify-center gap-1 pb-2">
      {heights.map((h, i) => (
        <View
          key={i}
          className={`w-[4] rounded-full ${working ? "bg-accent" : "bg-accent/80"}`}
          style={{ height: h }}
        />
      ))}
    </View>
  );
}

function StatusChip({ message, tone, working }: { message: string; tone: Tone; working: boolean }) {
  const tick = useTicker(450, working);
  const dimmed = working && tick % 2 === 1;

  if (message === "") return null;

  return (
    <View className="flex-row items-center gap-2 max-w-[340] h-[24] px-3 rounded-full bg-well border border-lineSoft overflow-hidden">
      <View className={`w-[6] h-[6] rounded-full ${toneDot[tone]} ${dimmed ? "opacity-30" : ""}`} />
      <Text className={`${toneText[tone]} text-[11]`}>{message}</Text>
    </View>
  );
}

function Label({ children }: { children: string }) {
  return <Text className="text-faint text-[10] font-bold tracking-widest">{children}</Text>;
}

function Switch({ on, disabled, onToggle }: { on: boolean; disabled: boolean; onToggle: () => void }) {
  return (
    <View
      className={`w-[40] h-[22] rounded-full relative border ${
        on ? "bg-accent border-accent" : "bg-well border-line hover:border-accent/60"
      } ${disabled ? "opacity-40" : "cursor-pointer"}`}
      onClick={disabled ? undefined : onToggle}
    >
      <View
        className={`absolute w-[16] h-[16] rounded-full top-[2] ${
          on ? "left-[21] bg-background" : "left-[2] bg-muted"
        }`}
      />
    </View>
  );
}

function DownloadButton({
  working,
  onClick,
}: {
  working: boolean;
  onClick: () => void;
}) {
  const tick = useTicker(350, working);
  const dots = working ? ".".repeat((tick % 3) + 1) : "";

  if (working) {
    return (
      <View className="w-[132] items-center justify-center rounded-lg bg-lift border border-line">
        <Text className="text-muted font-bold text-[12] tracking-widest">{`FETCHING${dots}`}</Text>
      </View>
    );
  }

  return (
    <View
      className="w-[132] items-center justify-center rounded-lg bg-accent cursor-pointer hover:bg-lime-200 active:bg-lime-500"
      style={{ shadowColor: "#C6F13544", shadowRadius: 18, shadowOffsetY: 3 }}
      onClick={onClick}
    >
      <Text className="text-background font-bold text-[12] tracking-widest">DOWNLOAD</Text>
    </View>
  );
}

function ProgressBar({ working }: { working: boolean }) {
  const tick = useTicker(80, working);

  return (
    <View className="h-[3] rounded-full bg-well overflow-hidden relative">
      {working ? (
        <View
          className="absolute top-0 h-full w-[140] rounded-full bg-accent"
          style={{ left: -140 + (tick * 24) % 880 }}
        />
      ) : null}
    </View>
  );
}

const fieldClasses =
  "bg-well border border-line focus:border-accent rounded-lg px-3 font-mono text-[13] text-text";
const fieldStyle = { placeholderColor: "#5A6253", caretColor: "#C6F135" };

export default function App() {
  const [version, setVersion] = useState("");
  const [url, setUrl] = useState("");
  const [clip, setClip] = useState(false);
  const [start, setStart] = useState("");
  const [end, setEnd] = useState("");
  const [working, setWorking] = useState(false);
  const [fileName, setFileName] = useState("");
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
    const offState = native.on("downloadState", (p) => setWorking(Boolean(p?.running)));
    const offDone = native.on("downloadFinished", (p) =>
      setFileName(p?.ok ? String(p?.fileName ?? "") : ""),
    );

    return () => {
      offStatus();
      offState();
      offDone();
    };
  }, []);

  const startDownload = () => {
    if (working) return;
    setFileName("");
    native.call("startDownload", { url: url.trim(), clip, start, end });
  };

  const ready = fileName !== "";

  return (
    <View className="w-full h-full bg-background p-4 gap-3 relative overflow-hidden">
      {/* faint vertical grid texture */}
      {[88, 176, 264, 352, 440, 528, 616].map((x) => (
        <View key={x} className="absolute top-0 bottom-0 w-px bg-line/30" style={{ left: x }} />
      ))}

      {/* Header */}
      <View className="flex-row items-center gap-3 px-1">
        <EqLogo working={working} />
        <View className="gap-px">
          <View className="flex-row items-center gap-2">
            <Text className="text-text text-[17] font-bold tracking-widest">STASHTRACK</Text>
            {version !== "" ? (
              <View className="px-2 h-[16] justify-center rounded-full border border-line bg-lift">
                <Text className="text-muted text-[9] font-mono">{`v${version}`}</Text>
              </View>
            ) : null}
          </View>
          <Text className="text-faint text-[10] tracking-wide">
            grab audio from anywhere · drag it into your DAW
          </Text>
        </View>
        <View className="flex-1" />
        <StatusChip message={status.message} tone={status.tone} working={working} />
      </View>

      {/* Source card */}
      <View
        className="rounded-2xl border border-line bg-panel p-4 gap-3"
        style={{ shadowColor: "#00000088", shadowRadius: 22, shadowOffsetY: 8 }}
      >
        <Label>SOURCE URL</Label>

        <View className="h-[42] flex-row gap-3">
          <TextInput
            className={`flex-1 h-full ${fieldClasses} ${working ? "opacity-50" : ""}`}
            style={fieldStyle}
            placeholder="https://www.youtube.com/watch?v=..."
            value={url}
            disabled={working}
            onChange={setUrl}
            onSubmit={startDownload}
          />
          <DownloadButton working={working} onClick={startDownload} />
        </View>

        <ProgressBar working={working} />

        <View className="flex-row items-center gap-3">
          <Switch on={clip} disabled={working} onToggle={() => setClip((c) => !c)} />
          <Label>CLIP RANGE</Label>
          <View className="flex-1" />
          <Label>START</Label>
          <TextInput
            className={`w-[96] h-[32] ${fieldClasses} ${!clip || working ? "opacity-40" : ""}`}
            style={fieldStyle}
            placeholder="0:30"
            value={start}
            disabled={!clip || working}
            onChange={setStart}
          />
          <Label>END</Label>
          <TextInput
            className={`w-[96] h-[32] ${fieldClasses} ${!clip || working ? "opacity-40" : ""}`}
            style={fieldStyle}
            placeholder="1:00"
            value={end}
            disabled={!clip || working}
            onChange={setEnd}
          />
        </View>
      </View>

      {/* Waveform card */}
      <View
        className="flex-1 rounded-2xl border border-line bg-panel p-4 gap-3"
        style={{ shadowColor: "#00000088", shadowRadius: 22, shadowOffsetY: 8 }}
      >
        <View className="flex-row items-center gap-2 h-[24] overflow-hidden">
          <Text
            className={`${ready ? "text-text" : "text-faint"} text-[12] font-bold tracking-wide`}
          >
            {ready ? fileName : "NO AUDIO LOADED"}
          </Text>
          {ready ? (
            <View className="px-2 h-[16] justify-center rounded-full bg-accent/15 border border-accent/30">
              <Text className="text-accent text-[9] font-bold tracking-widest">READY</Text>
            </View>
          ) : null}
          <View className="flex-1" />
          {ready ? (
            <View
              className="px-3 h-[22] justify-center rounded-full bg-accent"
              style={{ shadowColor: "#C6F13544", shadowRadius: 14, shadowOffsetY: 2 }}
            >
              <Text className="text-background text-[10] font-bold tracking-widest">
                DRAG TO PLAYLIST
              </Text>
            </View>
          ) : (
            <View className="px-3 h-[22] justify-center rounded-full border border-lineSoft">
              <Text className="text-faint text-[10] font-bold tracking-widest">WAITING</Text>
            </View>
          )}
        </View>

        <NativeView nativeId="waveform" className="flex-1" />
      </View>

      {/* Footer caption */}
      <View className="flex-row px-1">
        <View className="flex-1" />
        <Text className="text-faint text-[9] tracking-widest opacity-70">
          RENDERED NATIVELY BY VSREACT · REACT 18
        </Text>
      </View>
    </View>
  );
}
