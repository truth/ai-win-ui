import {
  Avatar,
  Badge,
  Breadcrumb,
  BreadcrumbItem,
  Button,
  Checkbox,
  Divider,
  Icon,
  Label,
  Pagination,
  Panel,
  ProgressBar,
  ScrollViewer,
  Slider,
  Switch,
  TextInput,
  TreeItem,
  TreeView,
  render,
  setWindowTitle,
  useEffect,
  useMemo,
  useState,
} from "ai-win-ui";

interface TaskItem {
  id: number;
  title: string;
  done: boolean;
  priority: "high" | "normal";
}

const EXPLORER_SECTIONS = [
  { key: "overview", label: "Architecture" },
  { key: "form", label: "Optimistic Form" },
  { key: "tasks", label: "Keyed Lists" },
  { key: "perf", label: "144Hz Stress" },
];

const INITIAL_TASKS: TaskItem[] = [
  { id: 1, title: "Zero RNW JS bridge bloat (QuickJS-ng C API)", done: true, priority: "high" },
  { id: 2, title: "Dual-thread CommitQueue + PostMessage batching", done: true, priority: "high" },
  { id: 3, title: "Optimistic UI input + eventSeq echo guard", done: true, priority: "normal" },
  { id: 4, title: "Typed Thickness {x, y} eliminating L,T,R,B bugs", done: true, priority: "normal" },
  { id: 5, title: "Keyed VNode diff straight onto C++ UIElement", done: false, priority: "high" },
  { id: 6, title: "Bun build millisecond TSX bundle pipeline", done: true, priority: "normal" },
  { id: 7, title: "Skia & Direct2D dual-backend validation", done: false, priority: "normal" },
];

export function App() {
  const [activeSection, setActiveSection] = useState("Architecture");
  const [selectedCount, setSelectedCount] = useState(3);

  // Optimistic Form state
  const [inputText, setInputText] = useState("Type fast here - 0ms optimistic input!");
  const [lastSubmitted, setLastSubmitted] = useState("(press Enter to submit)");
  const [darkHighlight, setDarkHighlight] = useState(true);
  const [agreeFlag, setAgreeFlag] = useState(true);
  const [progress, setProgress] = useState(64);

  // Keyed list + pagination state
  const [tasks, setTasks] = useState<TaskItem[]>(INITIAL_TASKS);
  const [nextTaskId, setNextTaskId] = useState(8);
  const [onlyOpen, setOnlyOpen] = useState(false);
  const [page, setPage] = useState(1);
  const pageSize = 4;

  // Dual-thread high-frequency ticker & JS isolation stress
  const [tickerOn, setTickerOn] = useState(false);
  const [tickCount, setTickCount] = useState(0);
  const [jsBusyMs, setJsBusyMs] = useState(0);

  useEffect(() => {
    setWindowTitle(`ai-win-ui · React JSX Showcase (${activeSection})`);
  }, [activeSection]);

  useEffect(() => {
    if (!tickerOn) return;
    const id = setInterval(() => {
      setTickCount((c) => c + 1);
      setProgress((p) => (p >= 100 ? 0 : p + 2));
    }, 16);
    return () => clearInterval(id);
  }, [tickerOn]);

  const filteredTasks = useMemo(() => {
    return onlyOpen ? tasks.filter((t) => !t.done) : tasks;
  }, [tasks, onlyOpen]);

  const totalPages = Math.max(1, Math.ceil(filteredTasks.length / pageSize));
  const safePage = Math.min(page, totalPages);
  const pageTasks = useMemo(() => {
    const start = (safePage - 1) * pageSize;
    return filteredTasks.slice(start, start + pageSize);
  }, [filteredTasks, safePage]);

  const addTask = () => {
    const id = nextTaskId;
    setNextTaskId(id + 1);
    setTasks((prev) => [
      { id, title: `Dynamic Task #${id} (key=${id})`, done: false, priority: id % 2 === 0 ? "high" : "normal" },
      ...prev,
    ]);
    setPage(1);
  };

  const toggleTask = (id: number) => {
    setTasks((prev) => prev.map((t) => (t.id === id ? { ...t, done: !t.done } : t)));
  };

  const removeTask = (id: number) => {
    setTasks((prev) => prev.filter((t) => t.id !== id));
  };

  const reverseTasks = () => {
    setTasks((prev) => [...prev].reverse());
  };

  const runJsCpuBurst = () => {
    const t0 = performance.now();
    let acc = 0;
    for (let i = 0; i < 800000; i++) {
      acc += (i * 17) % 13;
    }
    const elapsed = Math.round(performance.now() - t0);
    setJsBusyMs(elapsed + (acc >= 0 ? 0 : 1));
  };

  const accentHex = darkHighlight ? "#3B82F6" : "#10B981";

  return (
    <Panel
      id="reactRoot"
      direction="column"
      background="#0B121A"
      padding={{ x: 18, y: 16 }}
      spacing={12}
      alignItems="stretch"
    >
      {/* Top Header */}
      <Panel
        direction="row"
        height={48}
        background="#121D2C"
        borderColor="#22344A"
        border={1}
        cornerRadius={10}
        padding={{ x: 16, y: 8 }}
        alignItems="center"
        justifyContent="space-between"
      >
        <Panel direction="row" spacing={10} alignItems="center">
          <Icon name="code" size={18} color={accentHex} />
          <Label fontSize={15} bold color="#F8FAFC">
            ai-win-ui React TSX Runtime
          </Label>
          <Badge text="QuickJS-ng" variant="primary" dot />
          <Badge text="Zero RNW Bloat" variant="success" />
        </Panel>

        <Panel direction="row" spacing={12} alignItems="center">
          <Breadcrumb separator="/" spacing={8} fontSize={12.5} separatorColor="#64748B">
            <BreadcrumbItem
              text="Workspace"
              icon="home"
              color="#94A3B8"
              hoverColor="#38BDF8"
              onClick={() => setActiveSection("Workspace")}
            />
            <BreadcrumbItem
              text="React Engine"
              icon="folder"
              color="#94A3B8"
              hoverColor="#38BDF8"
              onClick={() => setActiveSection("React Engine")}
            />
            <BreadcrumbItem text={activeSection} color="#F8FAFC" active />
          </Breadcrumb>
          <Divider vertical height={18} color="#263A50" />
          <Avatar initials="QJ" size={28} status="online" background="#1E3A8A" color="#60A5FA" />
        </Panel>
      </Panel>

      {/* Main Split Workspace */}
      <Panel direction="row" flexGrow={1} spacing={12} alignItems="stretch">
        {/* Left Tree Sidebar */}
        <Panel
          direction="column"
          width={260}
          background="#121D2C"
          borderColor="#22344A"
          border={1}
          cornerRadius={10}
          padding={{ x: 12, y: 12 }}
          spacing={8}
          alignItems="stretch"
        >
          <Panel direction="row" justifyContent="space-between" alignItems="center" height={24}>
            <Panel direction="row" spacing={6} alignItems="center">
              <Icon name="folder" size={14} color="#F59E0B" />
              <Label fontSize={12} color="#94A3B8" bold>
                NATIVE TREE
              </Label>
            </Panel>
            <Badge text={`${selectedCount} checked`} variant="info" />
          </Panel>
          <Divider color="#22344A" />
          <TreeView
            id="navTree"
            flexGrow={1}
            showLines
            showCheckBox
            background="transparent"
            borderColor="#22344A"
            selectedBackground="#1E3A5F"
            hoverBackground="#16273C"
            color="#E2E8F0"
            lineColor="#263A50"
            glyphColor="#94A3B8"
            itemHeight={28}
            indentWidth={18}
            fontSize={12.5}
            onSelect={(label) => setActiveSection(label)}
            onCheck={(checked) => setSelectedCount((c) => Math.max(0, c + (checked ? 1 : -1)))}
          >
            <TreeItem header="react-runtime" icon="folder" expanded>
              <TreeItem header="QuickJS Engine" icon="database" checked />
              <TreeItem header="MutationBatch" icon="code" checked />
              <TreeItem header="Micro Reconciler" icon="code" checked />
            </TreeItem>
            <TreeItem header="Views & Demos" icon="folder" expanded>
              {EXPLORER_SECTIONS.map((s) => (
                <TreeItem key={s.key} header={s.label} icon="file" />
              ))}
            </TreeItem>
          </TreeView>
        </Panel>

        {/* Right Interactive Cards inside ScrollViewer */}
        <ScrollViewer flexGrow={1} scrollY background="transparent">
          <Panel direction="column" spacing={12} padding={{ right: 12 }} alignItems="stretch">
            {/* Card 1: Optimistic Dual-Thread Form */}
            <Panel
              direction="column"
              background="#121D2C"
              borderColor="#22344A"
              border={1}
              cornerRadius={10}
              padding={{ x: 16, y: 14 }}
              spacing={10}
              alignItems="stretch"
            >
              <Panel direction="row" justifyContent="space-between" alignItems="center" height={24}>
                <Label fontSize={14} bold color="#F8FAFC">
                  1. Optimistic UI Input + Dual-Thread Echo Suppression (eventSeq)
                </Label>
                <Label width={140} height={22} fontSize={12} color="#60A5FA" horizontalAlign="end">
                  Chars: {inputText.length}
                </Label>
              </Panel>

              <Panel direction="row" spacing={10} alignItems="center">
                <TextInput
                  id="optTextInput"
                  flexGrow={1}
                  height={34}
                  text={inputText}
                  placeholder="Type anything..."
                  background="#0E1724"
                  borderColor="#2A3E59"
                  focusBorderColor={accentHex}
                  color="#F1F5F9"
                  cornerRadius={6}
                  fontSize={13}
                  onTextChanged={(txt) => setInputText(txt)}
                  onSubmit={(txt) => setLastSubmitted(txt)}
                />
                <Button
                  text="Clear Text"
                  width={100}
                  height={34}
                  background="#1E293B"
                  foreground="#F8FAFC"
                  cornerRadius={6}
                  fontSize={12.5}
                  onClick={() => setInputText("")}
                />
                <Button
                  text="Preset"
                  width={84}
                  height={34}
                  background={accentHex}
                  foreground="#FFFFFF"
                  cornerRadius={6}
                  fontSize={12.5}
                  onClick={() => setInputText("Hello QuickJS + Yoga!")}
                />
              </Panel>

              <Label fontSize={12} color="#94A3B8">
                Last Enter Submit: {lastSubmitted}
              </Label>

              <Panel direction="row" spacing={16} alignItems="center" height={36}>
                <Switch
                  checked={darkHighlight}
                  label={darkHighlight ? "Accent: Blue" : "Accent: Emerald"}
                  activeColor={accentHex}
                  inactiveColor="#334155"
                  color="#E2E8F0"
                  onChange={(v) => setDarkHighlight(v)}
                />
                <Checkbox
                  checked={agreeFlag}
                  text="Batch Zero-Layout Updates"
                  color="#E2E8F0"
                  fontSize={13}
                  onChange={(v) => setAgreeFlag(v)}
                />
              </Panel>

              <Panel direction="row" spacing={12} alignItems="center" height={42}>
                <Slider
                  flexGrow={1}
                  min={0}
                  max={100}
                  step={1}
                  value={progress}
                  text="Slider"
                  background="#0E1724"
                  borderColor="#2A3E59"
                  highlightColor={accentHex}
                  color="#E2E8F0"
                  onChange={(val) => setProgress(Math.round(val))}
                />
                <ProgressBar
                  width={260}
                  height={32}
                  min={0}
                  max={100}
                  value={progress}
                  showValueText
                  background="#0E1724"
                  borderColor="#2A3E59"
                  trackColor="#1E293B"
                  fillColor={accentHex}
                  color="#F8FAFC"
                />
              </Panel>
            </Panel>

            {/* Card 2: Keyed Reconciliation + Pagination */}
            <Panel
              direction="column"
              background="#121D2C"
              borderColor="#22344A"
              border={1}
              cornerRadius={10}
              padding={{ x: 16, y: 14 }}
              spacing={10}
              alignItems="stretch"
            >
              <Panel direction="row" justifyContent="space-between" alignItems="center" height={32}>
                <Label fontSize={14} bold color="#F8FAFC">
                  2. Keyed Reconciliation & Pagination ({filteredTasks.length} items)
                </Label>
                <Panel direction="row" spacing={8} alignItems="center">
                  <Checkbox
                    checked={onlyOpen}
                    text="Only Open"
                    color="#CBD5E1"
                    fontSize={12.5}
                    onChange={(v) => {
                      setOnlyOpen(v);
                      setPage(1);
                    }}
                  />
                  <Button
                    text="Reverse"
                    width={78}
                    height={28}
                    background="#1E293B"
                    foreground="#E2E8F0"
                    cornerRadius={6}
                    fontSize={12}
                    onClick={reverseTasks}
                  />
                  <Button
                    text="+ Add Item"
                    width={96}
                    height={28}
                    background={accentHex}
                    foreground="#FFFFFF"
                    cornerRadius={6}
                    fontSize={12}
                    onClick={addTask}
                  />
                </Panel>
              </Panel>

              <Panel direction="column" spacing={6} alignItems="stretch">
                {pageTasks.map((task) => (
                  <Panel
                    key={task.id}
                    direction="row"
                    height={36}
                    background="#0E1724"
                    borderColor="#1E2D40"
                    border={1}
                    cornerRadius={6}
                    padding={{ x: 10, y: 4 }}
                    alignItems="center"
                    justifyContent="space-between"
                  >
                    <Panel direction="row" spacing={8} alignItems="center" flexGrow={1}>
                      <Checkbox
                        checked={task.done}
                        text={task.title}
                        color={task.done ? "#64748B" : "#E2E8F0"}
                        fontSize={12.5}
                        onChange={() => toggleTask(task.id)}
                      />
                    </Panel>
                    <Panel direction="row" spacing={8} alignItems="center">
                      <Badge
                        text={task.priority}
                        variant={task.priority === "high" ? "warning" : "neutral"}
                      />
                      <Button
                        text="Delete"
                        width={60}
                        height={24}
                        background="#3F1D2B"
                        foreground="#FDA4AF"
                        cornerRadius={5}
                        fontSize={11.5}
                        onClick={() => removeTask(task.id)}
                      />
                    </Panel>
                  </Panel>
                ))}
              </Panel>

              <Panel direction="row" justifyContent="end" alignItems="center" height={34}>
                <Pagination
                  current={safePage}
                  total={filteredTasks.length}
                  pageSize={pageSize}
                  showTotal
                  activeColor={accentHex}
                  color="#CBD5E1"
                  borderColor="#263A50"
                  hoverColor="#1E293B"
                  onPageChange={(nextPage) => setPage(nextPage)}
                />
              </Panel>
            </Panel>

            {/* Card 3: Dual-Thread Smoothness & 60Hz Visual-Only Ticker */}
            <Panel
              direction="column"
              background="#121D2C"
              borderColor="#22344A"
              border={1}
              cornerRadius={10}
              padding={{ x: 16, y: 14 }}
              spacing={10}
              alignItems="stretch"
            >
              <Panel direction="row" justifyContent="space-between" alignItems="center" height={28}>
                <Label fontSize={14} bold color="#F8FAFC">
                  3. Dual-Thread 60Hz Visual-Only Fast Path (Press F12 for Profiler)
                </Label>
                <Label width={140} height={22} fontSize={12.5} color="#34D399" horizontalAlign="end">
                  Ticks: {tickCount}
                </Label>
              </Panel>

              <Panel direction="row" spacing={12} alignItems="center">
                <Switch
                  checked={tickerOn}
                  label={tickerOn ? "60Hz Ticker RUNNING" : "Start 60Hz Ticker"}
                  activeColor="#10B981"
                  inactiveColor="#334155"
                  color="#E2E8F0"
                  onChange={(v) => setTickerOn(v)}
                />
                <Button
                  text="Run Heavy JS Burst (UI Stays Smooth)"
                  width={260}
                  height={32}
                  background="#1E293B"
                  foreground="#F8FAFC"
                  cornerRadius={6}
                  fontSize={12}
                  onClick={runJsCpuBurst}
                />
                <Label width={200} height={22} fontSize={12} color="#94A3B8">
                  Last JS Burst: {jsBusyMs} ms
                </Label>
              </Panel>
            </Panel>
          </Panel>
        </ScrollViewer>
      </Panel>
    </Panel>
  );
}

render(<App />);

