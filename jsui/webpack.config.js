const webpack = require("webpack");

module.exports = {
  entry: "./src/index.jsx",
  output: {
    path: __dirname + "/build/js",
    filename: "main.js",
    sourceMapFilename: "[file].map",
    devtoolModuleFilenameTemplate: (info) =>
      `webpack:///${info.absoluteResourcePath.replace(/\\/g, "/")}`,
  },
  target: ["web", "es5"],
  devtool: "source-map",
  resolve: {
    extensions: [".js", ".jsx", ".json"],
  },
  module: {
    rules: [
      {
        test: /\.(js|jsx)$/,
        exclude: /node_modules/,
        use: ["babel-loader"],
      },
    ],
  },
  plugins: [
    new webpack.DefinePlugin({
      "process.env.NODE_DEBUG": JSON.stringify(process.env.NODE_DEBUG),
    }),
  ],
};
