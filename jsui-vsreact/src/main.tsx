import { render, configureTheme } from "@vsreact/core";
import App from "./App";

configureTheme({
  colors: {
    background: "#060806",
    panel: "#0D100C",
    lift: "#12160F",
    well: "#090B08",
    line: "#242B20",
    lineSoft: "#1A1F17",
    accent: "#C6F135",
    text: "#EDF1E4",
    muted: "#848D7B",
    faint: "#5A6253",
    error: "#FF5D6C",
  },
});

render(<App />);
