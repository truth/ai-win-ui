import {
  Label,
  Panel,
  TextInput,
  render,
  setWindowTitle,
  useEffect,
  useState,
} from "ai-win-ui";

const ROWS = 10;
const COLS = 10;

function computeCells(tick: number): { values: string[]; sum: number } {
  const values: string[] = new Array(100);
  let sum = 0;
  for (let i = 0; i < 100; i++) {
    const v = 100.0 + ((tick * 3 + i) % 100) * 0.13;
    sum += v;
    values[i] = v.toFixed(2);
  }
  return { values, sum };
}

export function BenchStressApp() {
  const [tick, setTick] = useState(0);
  const [inputText, setInputText] = useState("");

  useEffect(() => {
    setWindowTitle("React 60Hz Stress Board (100 Cells + Rapid Typing)");
  }, []);

  useEffect(() => {
    const id = setInterval(() => {
      setTick((t) => t + 1);
    }, 16);
    return () => clearInterval(id);
  }, []);

  const board = computeCells(tick);

  return (
    <Panel direction="column" padding={10} spacing={6} background="#0B111A" alignItems="stretch">
      <Panel direction="row" height={32} padding={0} spacing={10} alignItems="center">
        <Label
          text="XML vs React 60Hz Stress Board (100 Fixed Cells + Rapid Typing)"
          fontSize={14}
          color="#E2E8F0"
          bold
          width={430}
          height={22}
        />
        <Label
          id="stress_tick_label"
          text={`Ticks: ${tick} | Sum: ${board.sum.toFixed(2)}`}
          fontSize={13}
          color="#38BDF8"
          width={210}
          height={22}
        />
        <TextInput
          id="stress_input"
          text={inputText}
          placeholder="Type rapid characters..."
          width={300}
          height={26}
          fontSize={12}
          background="#162032"
          color="#F8FAFC"
          borderColor="#334155"
          onTextChanged={setInputText}
        />
      </Panel>

      <Panel direction="column" padding={4} spacing={4} background="#0F172A" alignItems="stretch">
        {Array.from({ length: ROWS }, (_, r) => (
          <Panel key={r} direction="row" height={24} padding={0} spacing={4} alignItems="center">
            {Array.from({ length: COLS }, (_, c) => {
              const idx = r * COLS + c;
              return (
                <Label
                  key={c}
                  id={`cell_${idx}`}
                  text={board.values[idx]}
                  fontSize={12}
                  color="#22C55E"
                  width={92}
                  height={22}
                />
              );
            })}
          </Panel>
        ))}
      </Panel>
    </Panel>
  );
}

render(<BenchStressApp />);

