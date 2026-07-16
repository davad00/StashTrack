import { render, configureTheme } from "@vsreact/core";
import App from "./App";

configureTheme({
  colors: {
    background: "#0A0B0A",
    panel: "#101210",
    panelLift: "#14170F",
    well: "#0C0E0C",
    line: "#2B3029",
    accent: "#C6F135",
    text: "#E8EAE6",
    muted: "#9AA097",
    error: "#FF4F64",
  },
});

render(<App />);
