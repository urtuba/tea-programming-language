# Tea Programming Language
Implementation <strong> a programming language </strong> named <strong>Tea</strong>. Exercise related to this topics:
<ul>
  <li><strong>Recursive Programming</strong></li>
  <li><strong>C++ (as language)</strong></li>
  <li><strong>2 Dimensional Arrays</strong></li>
  <li><strong>Struct Templates</strong></li>
  <li><strong>Stack Implementation</strong></li>
  <li><strong>Data Structures</strong></li>
</ul>

## Problem
A company's engineers decided to write simple programming language. That language would be easy to write, so has simple rules. <b>Tea</b> programs consist of subsequent lines of statements. Each statement has a simple job. There are 7 statements and 5 variables.

### Statements
<dl>
  <dt>inc</dt>
  <dd>Increments arg1 by arg2</dd>
  <dt>dec</dt>
  <dd>Decrements arg2 by arg2</dd>
  <dt>mul</dt>
  <dd>Multiplies arg1 by arg2</dd>
  <dt>div</dt>
  <dd>Divides arg1 by arg2</dd>
  <dt>function</dt>
  <dd>Defines a function named arg1</dd>
  <dt>call</dt>
  <dd>Calls arg1 with argument arg2</dd>
  <dt>return</dt>
  <dd>returns arg1</dd>
</dl>

#### Variables
Engineers of the company all hated local variable declerations. So, <b>Tea</b> does not allow define variables. There are 5 predefined variables: [a,b,c,d,e]. A function can use these variables by accessing and operating on them. They all specific to a function. Every function uses different [a,b,c,d,e] variables. They are all 0 at initialization.

### Functions
Functions are defined like;
<div style="background-color: lightgrey"><pre>
  function foo
  <em>//operations with call, mul, inc, div, dec</em>
  return a <em> // or any variable </em>
</pre></div>
Functions are called like;

<div style="background-color: lightgrey">
call foo e
</div>

This function call initializes all variables for this call except <b><em>e</em></b>. <b><em>e</em></b> is taken by parent functions variable. (But it initializes again for new function, changes on it does not change parent's <b><em>e</em></b>). Due to <b>return a</b> statement, function finally assigns its <b><em>a</em></b> to the parent's <b><em>a</em></b> variable.
Each Tea program has a <strong>main function</strong> and starts from this function. It is not necessity to place functions in certain order. A function can be defined and called before or after another functions including main.

### Input / Coding with tea
<div style="background-color: black, color: white">

#### example2.tea
<pre>
  function main
  inc a 2
  inc b 3
  call foo a
  call bar b
  inc d a
  inc d c
  return d
  function foo
  inc a 4
  div a 2
  return a
  function bar
  inc c 7
  dec c b
  return c
</pre></div>

### workflow of example2.tea

<em>Lines are 0-indexed</em>
<table>
  <thead>
    <th>Executed Line</th>
    <th>Next Line</th>
    <th>[a,b,c,d,e]</th>
  </thead>
  <tr>
    <td>0</td>
    <td>1</td>
    <td>0,0,0,0,0</td>
  </tr>
  <tr>
    <td>1</td>
    <td>2</td>
    <td>2,0,0,0,0</td>
  </tr>
  <tr>
    <td>2</td>
    <td>3</td>
    <td>2,3,0,0,0</td>
  </tr>
  <tr>
    <td>3</td>
    <td>9</td>
    <td>2,0,0,0,0</td>
  </tr>
  <tr>
    <td>9</td>
    <td>10</td>
    <td>6,0,0,0,0</td>
  </tr>
  <tr>
    <td>10</td>
    <td>11</td>
    <td>3,0,0,0,0</td>
  </tr>
  <tr>
    <td>11</td>
    <td>4</td>
    <td>3,3,0,0,0</td>
  </tr>
  <tr>
    <td>4</td>
    <td>13</td>
    <td>0,3,0,0,0</td>
  </tr>
  <tr>
    <td>13</td>
    <td>14</td>
    <td>0,3,7,0,0</td>
  </tr>
  <tr>
    <td>14</td>
    <td>15</td>
    <td>0,3,4,0,0</td>
  </tr>
  <tr>
    <td>15</td>
    <td>5</td>
    <td>3,3,4,0,0</td>
  </tr>
  <tr>
    <td>5</td>
    <td>6</td>
    <td>3,3,4,3,0</td>
  </tr>
  <tr>
    <td>6</td>
    <td>7</td>
    <td>3,3,4,7,0</td>
  </tr>
  <tr>
    <td>7</td>
    <td> </td>
    <td>3,3,4,<b>7</b>,0</td>
  </tr>
</table>
Then program outputs 7.

### How to Run
compile:
```
g++ tea.cpp -o tea.exe
```
run:
```
tea.exe examples/example1.tea
```
### Notes

As I code this program, there is a problem with variables. They should initialize at start but every function uses new set of variables. Also functions can call functions, so i cannot use just one set of inputs for main, one for called functions. I needed segmentated memory. But I had limitation which is set by my instructor. In this limits:

I implemented variables as 2D array: <code> T variables[LANGUAGE_DEPTH][5]; </code>. Which means I have <strong>LANGUAGE_DEPTH</strong> times array of 5 variables. Variables[0] is set of variables of main function. If main calls a function i use Variables[1] for it. If it calls another, program uses Variables[2]. I say this <b><em>k</em></b> number in Variables[<b><em>k</em></b>] depth or degree of function. I use <strong>LANGUAGE_DEPTH</strong> property to scale power of <strong>Tea programming language.</strong> In other words, capability to work with nested functions. You can have more powerfull tea engine with increasing it. (Default: 5).
