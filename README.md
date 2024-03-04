joymouse
========

Joy-Con(L) をマウスとして使うためのシンプルなプログラムです。  
Joy-Con(R) は未対応です。

ジャイロなどのセンサー情報も利用せず、スティックとボタンだけのシンプルな操作を想定しています。

使い方
------

Joy-Con(L) を Bluetooth で接続している状態でこのプログラムを起動するだけです。
そのまま起動すると縦持ち、起動パラメーターに `--horz` を指定すると横持ちになります。

スティックでマウスカーソル移動、ボタンでちょっとしたキーボード操作が行えるようになります。

なお、通常起動だと UAC のダイアログなどのように一般のユーザー権限では操作できないウィンドウは触れません。  
単にブラウザーだけを操作する場合など、用途が限られているなら特に支障はありません。

UAC のダイアログなども操作したい場合は、このプログラムを管理者として実行してください。

キー割り当て
--------

`ホイールトリガー` は押している間、スティック操作がホイール入力に切り替わります。  
`f キー` はキーボードの `f` を入力するキーで、多くの動画サイトでフルスクリーン化のショートカットキーになっています。

### 縦持ち

|Joy-Con(L) | キー |
|------------|------|
|ZL|左クリック|
|マイナス|右クリック|
|L|ホイールトリガー|
|スティック押し込み|ホイールクリック|
|←|ブラウザーバック|
|↑|f キー|
|↓|タッチキーボードの表示切り替え|
|SL|音量を上げる|
|SR|音量を下げる|

### 横持ち

|Joy-Con(L) | キー |
|------------|------|
|↓|左クリック|
|→|右クリック|
|↑|ホイールトリガー|
|スティック押し込み|ホイールクリック|
|マイナス|ブラウザーバック|
|L|f キー|
|←|タッチキーボードの表示切り替え|
|SR|音量を上げる|
|SL|音量を下げる|

Credits
-------

joymouse is made possible by the following open source softwares.

### [JoyShockLibrary](https://github.com/JibbSmart/JoyShockLibrary)

<details>
<summary>The MIT License</summary>

```
Copyright 2018-2023 Julian Smart

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
</details>