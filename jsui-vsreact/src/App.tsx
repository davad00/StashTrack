import { useEffect, useRef, useState, type ReactNode } from "react";
import {
  View,
  Text,
  TextInput,
  NativeView,
  native,
  useTween,
  lerp,
  Easing,
} from "@vsreact/core";

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

function ProgressBar({ working, percent }: { working: boolean; percent: number | null }) {
  const tick = useTicker(80, working && percent === null);

  return (
    <View className="h-[3] rounded-full bg-well overflow-hidden relative">
      {working && percent !== null ? (
        // Real yt-dlp progress.
        <View
          className="absolute top-0 left-0 h-full rounded-full bg-accent"
          style={{ width: `${Math.max(1, percent)}%` }}
        />
      ) : working ? (
        // Indeterminate sweep until the first percent arrives.
        <View
          className="absolute top-0 h-full w-[140] rounded-full bg-accent"
          style={{ left: -140 + (tick * 24) % 880 }}
        />
      ) : null}
    </View>
  );
}

/** Preview position bar — drag to scrub. */
const SEEK_BAR_WIDTH = 656; // card inner width at the fixed 720px window

function SeekBar({
  fraction,
  onSeek,
}: {
  fraction: number;
  onSeek: (fraction: number) => void;
}) {
  const startFraction = useRef(0);

  return (
    <View
      className="h-[12] justify-center cursor-pointer"
      onDragStart={() => {
        startFraction.current = fraction;
      }}
      onDrag={(e) =>
        onSeek(Math.min(1, Math.max(0, startFraction.current + e.dx / SEEK_BAR_WIDTH)))
      }
    >
      <View className="h-[3] rounded-full bg-well overflow-hidden">
        <View
          className="h-full rounded-full bg-accent/70"
          style={{ width: `${fraction * 100}%` }}
        />
      </View>
    </View>
  );
}

interface StashEntry {
  path: string;
  name: string;
  addedMs: number;
}

function formatAge(addedMs: number): string {
  const minutes = Math.max(0, Math.round((Date.now() - addedMs) / 60000));
  if (minutes < 1) return "just now";
  if (minutes < 60) return `${minutes}m ago`;
  const hours = Math.round(minutes / 60);
  if (hours < 24) return `${hours}h ago`;
  return `${Math.round(hours / 24)}d ago`;
}

function StashDrawer({
  entries,
  onClose,
  onLoad,
  onRemove,
}: {
  entries: StashEntry[];
  onClose: () => void;
  onLoad: (entry: StashEntry) => void;
  onRemove: (entry: StashEntry) => void;
}) {
  const t = useTween({ duration: 260, easing: Easing.outCubic });

  return (
    <View className="absolute inset-0">
      <View
        className="absolute inset-0 bg-black cursor-pointer"
        style={{ opacity: 0.55 * t }}
        onClick={onClose}
      />
      <View
        className="absolute top-0 bottom-0 w-[300] bg-panel border border-line p-4 gap-3"
        style={{ right: lerp(-310, 0, t), shadowColor: "#000000AA", shadowRadius: 30, shadowOffsetY: 0 }}
      >
        <View className="flex-row items-center">
          <Text className="text-text text-[13] font-bold tracking-widest">STASH</Text>
          <View className="px-2 h-[16] ml-2 justify-center rounded-full border border-line bg-lift">
            <Text className="text-muted text-[9] font-mono">{String(entries.length)}</Text>
          </View>
          <View className="flex-1" />
          <View
            className="w-[22] h-[22] rounded-full border border-line items-center justify-center cursor-pointer hover:border-accent/70"
            onClick={onClose}
          >
            <Text className="text-muted text-[10] font-bold">x</Text>
          </View>
        </View>

        <Text className="text-faint text-[9] tracking-wide">
          your downloads — click one to reload it
        </Text>

        <View className="flex-1 overflow-y-scroll gap-2 pr-2">
          {entries.length === 0 ? (
            <Text className="text-faint text-[11]">nothing stashed yet</Text>
          ) : (
            entries.map((entry) => (
              <View
                key={entry.path}
                className="rounded-lg border border-lineSoft bg-lift p-3 gap-1 cursor-pointer hover:border-accent/50"
                onClick={() => onLoad(entry)}
              >
                <View className="h-[15] overflow-hidden">
                  <Text className="text-text text-[11] font-bold">{entry.name}</Text>
                </View>
                <View className="flex-row items-center">
                  <Text className="text-faint text-[9] tracking-wide">{formatAge(entry.addedMs)}</Text>
                  <View className="flex-1" />
                  <View
                    className="px-2 h-[15] justify-center rounded-full border border-lineSoft cursor-pointer hover:border-error/80"
                    onClick={() => onRemove(entry)}
                  >
                    <Text className="text-faint text-[8] font-bold tracking-widest">REMOVE</Text>
                  </View>
                </View>
              </View>
            ))
          )}
        </View>
      </View>
    </View>
  );
}

const fieldClasses =
  "bg-well border border-line focus:border-accent rounded-lg px-3 font-mono text-[13] text-text";
const fieldStyle = { placeholderColor: "#5A6253", caretColor: "#C6F135" };

//==============================================================================
// Splash

/** One rising splash bar with a staggered, overshooting entrance. */
function SplashBar({ height, delay }: { height: number; delay: number }) {
  const t = useTween({ duration: 620, delay, easing: Easing.outBack });

  return (
    <View
      className="w-[7] rounded-full bg-accent"
      style={{ height: Math.max(2, lerp(2, height, t)), opacity: Math.min(1, t * 2) }}
    />
  );
}

/** One wordmark letter fading up into place. */
function SplashLetter({ letter, index }: { letter: string; index: number }) {
  const t = useTween({ duration: 460, delay: 420 + index * 45, easing: Easing.outCubic });

  return (
    <Text
      className="text-text text-[24] font-bold tracking-widest"
      style={{ opacity: t, marginTop: lerp(12, 0, t) }}
    >
      {letter}
    </Text>
  );
}

function Splash({
  version,
  onReveal,
  onDone,
}: {
  version: string;
  onReveal: () => void;
  onDone: () => void;
}) {
  // Logo tile glow breathes in.
  const glow = useTween({ duration: 900, easing: Easing.outCubic });
  // Accent hairline sweeps out from the middle.
  const line = useTween({ duration: 550, delay: 1050, easing: Easing.outQuint });
  // Caption fade.
  const caption = useTween({ duration: 420, delay: 1250, easing: Easing.outCubic });
  // Fade the splash CONTENT to black first, then unmount and let the main UI
  // stagger in on a clean backdrop — no mid-fade bleed-through.
  const fade = useTween({
    duration: 380,
    delay: 2050,
    easing: Easing.inOutCubic,
    onComplete: () => {
      onReveal();
      onDone();
    },
  });

  const skip = () => {
    onReveal();
    onDone();
  };

  return (
    <View
      className="absolute inset-0 bg-background items-center justify-center cursor-pointer"
      style={{ opacity: 1 }}
      onClick={skip}
    >
    <View className="items-center justify-center" style={{ opacity: 1 - fade }}>
      {/* logo tile */}
      <View
        className="w-[86] h-[86] rounded-2xl bg-accent/10 border border-accent/25 flex-row items-end justify-center gap-[6] pb-[18]"
        style={{
          shadowColor: "#C6F13530",
          shadowRadius: lerp(4, 34, glow),
          shadowOffsetY: 0,
          opacity: Math.min(1, glow * 2.5),
        }}
      >
        <SplashBar height={22} delay={120} />
        <SplashBar height={40} delay={240} />
        <SplashBar height={30} delay={360} />
      </View>

      {/* wordmark */}
      <View className="flex-row mt-5 h-[30]">
        {"STASHTRACK".split("").map((letter, i) => (
          <SplashLetter key={i} letter={letter} index={i} />
        ))}
      </View>

      {/* hairline sweep */}
      <View className="h-[2] rounded-full bg-accent mt-4" style={{ width: lerp(0, 190, line) }} />

      {/* caption */}
      <View className="flex-row items-center gap-2 mt-4" style={{ opacity: caption }}>
        <Text className="text-faint text-[10] tracking-widest">N9 RECORDS</Text>
        {version !== "" ? (
          <View className="px-2 h-[16] justify-center rounded-full border border-line bg-lift">
            <Text className="text-muted text-[9] font-mono">{`v${version}`}</Text>
          </View>
        ) : null}
      </View>
    </View>
    </View>
  );
}

/** Fade-and-rise entrance wrapper for the main UI sections. */
function Enter({
  delay,
  className,
  children,
}: {
  delay: number;
  className?: string;
  children?: ReactNode;
}) {
  const t = useTween({ duration: 520, delay, easing: Easing.outCubic });

  return (
    <View className={className} style={{ opacity: t, marginTop: lerp(14, 0, t) }}>
      {children}
    </View>
  );
}

export default function App() {
  const [version, setVersion] = useState("");
  const [url, setUrl] = useState("");
  const [clip, setClip] = useState(false);
  const [start, setStart] = useState("");
  const [end, setEnd] = useState("");
  const [working, setWorking] = useState(false);
  const [fileName, setFileName] = useState("");
  const [percent, setPercent] = useState<number | null>(null);
  const [preview, setPreview] = useState({ playing: false, fraction: 0 });
  const [stash, setStash] = useState<StashEntry[]>([]);
  const [stashOpen, setStashOpen] = useState(false);
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

    setStash((native.call("history:get") as StashEntry[]) ?? []);

    const offStatus = native.on("status", (p) =>
      setStatus({ message: String(p?.message ?? ""), tone: (p?.tone as Tone) ?? "muted" }),
    );
    const offState = native.on("downloadState", (p) => {
      setWorking(Boolean(p?.running));
      if (p?.running) setPercent(null);
    });
    const offProgress = native.on("downloadProgress", (p) => setPercent(Number(p?.percent ?? 0)));
    const offPreview = native.on("preview", (p) =>
      setPreview({ playing: Boolean(p?.playing), fraction: Number(p?.fraction ?? 0) }),
    );
    const offDone = native.on("downloadFinished", (p) => {
      setFileName(p?.ok ? String(p?.fileName ?? "") : "");
      if (p?.ok) setStash((native.call("history:get") as StashEntry[]) ?? []);
    });

    return () => {
      offStatus();
      offState();
      offProgress();
      offPreview();
      offDone();
    };
  }, []);

  const loadStashEntry = (entry: StashEntry) => {
    const result = native.call("history:load", { path: entry.path });
    if (result?.ok) setFileName(String(result.fileName ?? entry.name));
    else setStash((native.call("history:get") as StashEntry[]) ?? []);
  };

  const removeStashEntry = (entry: StashEntry) => {
    setStash((native.call("history:remove", { path: entry.path }) as StashEntry[]) ?? []);
  };

  const startDownload = () => {
    if (working) return;
    setFileName("");
    native.call("startDownload", { url: url.trim(), clip, start, end });
  };

  const ready = fileName !== "";
  const [revealed, setRevealed] = useState(false);
  const [splashGone, setSplashGone] = useState(false);

  return (
    <View className="w-full h-full bg-background p-4 gap-3 relative overflow-hidden">
      {/* faint vertical grid texture */}
      {[88, 176, 264, 352, 440, 528, 616].map((x) => (
        <View key={x} className="absolute top-0 bottom-0 w-px bg-line/30" style={{ left: x }} />
      ))}

      {revealed ? (
      <>
      {/* Header */}
      <Enter delay={0} className="flex-row items-center gap-3 px-1">
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
        <View
          className="px-3 h-[24] justify-center rounded-full border border-line bg-lift cursor-pointer hover:border-accent/70"
          onClick={() => {
            setStash((native.call("history:get") as StashEntry[]) ?? []);
            setStashOpen(true);
          }}
        >
          <Text className="text-muted text-[10] font-bold tracking-widest">
            {`STASH ${stash.length}`}
          </Text>
        </View>
      </Enter>

      {/* Source card */}
      <Enter delay={110}>
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

        <ProgressBar working={working} percent={percent} />

        <View className="flex-row items-center gap-3">
          <Switch on={clip} disabled={working} onToggle={() => setClip((c) => !c)} />
          <Label>CLIP RANGE</Label>
          <View className="flex-1" />
          <Label>START</Label>
          <TextInput
            className={`w-[96] h-[32] ${fieldClasses} ${
              stashOpen ? "opacity-0" : !clip || working ? "opacity-40" : ""
            }`}
            style={fieldStyle}
            placeholder="0:30"
            value={start}
            disabled={!clip || working}
            onChange={setStart}
          />
          <Label>END</Label>
          <TextInput
            className={`w-[96] h-[32] ${fieldClasses} ${
              stashOpen ? "opacity-0" : !clip || working ? "opacity-40" : ""
            }`}
            style={fieldStyle}
            placeholder="1:00"
            value={end}
            disabled={!clip || working}
            onChange={setEnd}
          />
        </View>
      </View>
      </Enter>

      {/* Waveform card */}
      <Enter delay={220} className="flex-1">
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
              className={`px-3 h-[22] justify-center rounded-full cursor-pointer ${
                preview.playing
                  ? "bg-accent"
                  : "border border-accent/40 hover:border-accent"
              }`}
              onClick={() => {
                const result = native.call("preview:toggle");
                if (result)
                  setPreview((p) => ({ ...p, playing: Boolean(result.playing) }));
              }}
            >
              <Text
                className={`${
                  preview.playing ? "text-background" : "text-accent"
                } text-[10] font-bold tracking-widest`}
              >
                {preview.playing ? "PAUSE" : "PLAY"}
              </Text>
            </View>
          ) : null}
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

        {ready ? (
          <SeekBar
            fraction={preview.fraction}
            onSeek={(fraction) => native.call("preview:seek", { fraction })}
          />
        ) : null}
      </View>
      </Enter>

      {/* Footer caption */}
      <Enter delay={330} className="flex-row px-1">
        <View className="flex-1" />
        <Text className="text-faint text-[9] tracking-widest opacity-70">
          RENDERED NATIVELY BY VSREACT · REACT 18
        </Text>
      </Enter>
      {stashOpen ? (
        <StashDrawer
          entries={stash}
          onClose={() => setStashOpen(false)}
          onLoad={loadStashEntry}
          onRemove={removeStashEntry}
        />
      ) : null}
      </>
      ) : null}

      {splashGone ? null : (
        <Splash
          version={version}
          onReveal={() => setRevealed(true)}
          onDone={() => setSplashGone(true)}
        />
      )}
    </View>
  );
}
