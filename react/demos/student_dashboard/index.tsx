import {
  createElement,
  useState,
  useMemo,
  render,
  setWindowTitle,
} from "../../runtime/index.ts";
import { Panel, Label, Button, Icon } from "../../runtime/index.ts";

import { Sidebar } from "./components/Sidebar.tsx";
import { TopBar } from "./components/TopBar.tsx";
import { HeroBanner } from "./components/HeroBanner.tsx";
import { PerformanceCard } from "./components/PerformanceCard.tsx";
import { MyVisitCard } from "./components/MyVisitCard.tsx";
import { LinkedTeachersCard } from "./components/LinkedTeachersCard.tsx";
import { CalendarCard } from "./components/CalendarCard.tsx";
import { UpcomingEventsCard } from "./components/UpcomingEventsCard.tsx";

import {
  NAV_TABS,
  PERFORMANCE_DATA,
  VISIT_DATA,
  CALENDAR_EVENTS,
} from "./data/mockData.ts";

export function StudentDashboardApp() {
  const [activeTab, setActiveTab] = useState("dashboard");
  const [searchText, setSearchText] = useState("");
  const [language, setLanguage] = useState("ENG");
  const [month, setMonth] = useState("December");
  const [calendarFilter, setCalendarFilter] = useState("Today");
  const [showAllLessons, setShowAllLessons] = useState(false);
  const [activeEventId, setActiveEventId] = useState("c1");
  const [statusMessage, setStatusMessage] = useState("");

  const toggleMonth = () => {
    setMonth((m) => (m === "December" ? "November" : "December"));
  };

  const toggleLanguage = () => {
    setLanguage((l) => (l === "ENG" ? "CHN" : "ENG"));
  };

  const performanceData = useMemo(() => {
    const raw = PERFORMANCE_DATA[month] || PERFORMANCE_DATA.December;
    if (!searchText.trim()) return raw;
    const q = searchText.trim().toLowerCase();
    const filtered = raw.filter((p) => p.subject.toLowerCase().includes(q));
    return filtered.length > 0 ? filtered : raw;
  }, [month, searchText]);

  const visitData = useMemo(() => {
    const raw = VISIT_DATA[month] || VISIT_DATA.December;
    if (!searchText.trim()) return raw;
    const q = searchText.trim().toLowerCase();
    const filtered = raw.filter((v) => v.subject.toLowerCase().includes(q));
    return filtered.length > 0 ? filtered : raw;
  }, [month, searchText]);

  const activeTabInfo = NAV_TABS.find((t) => t.id === activeTab) || NAV_TABS[0];

  return (
    <Panel
      direction="row"
      flexGrow={1}
      flexBasis={0}
      windowChrome="custom"
      background="#3D509B"
      padding={0}
      spacing={0}
      alignItems="stretch"
    >
      {/* Left Dual-Tone Sidebar */}
      <Sidebar
        activeTab={activeTab}
        onSelectTab={(tabId) => {
          setActiveTab(tabId);
          setStatusMessage("");
          setWindowTitle(`Dashboard for student - ${tabId}`);
        }}
        onLogOut={() => {
          setStatusMessage("Logged out Grace Stanley session (demo)");
        }}
      />

      {/* Right Main Canvas (#F1F3F9 Light Mode) */}
      <Panel
        direction="column"
        flexGrow={1}
        flexBasis={0}
        minWidth={520}
        background="#F1F3F9"
        padding={{ left: 16, top: 8, right: 16, bottom: 10 }}
        spacing={10}
        alignItems="stretch"
      >
        {/* Top Bar */}
        <TopBar
          searchText={searchText}
          onSearchChange={setSearchText}
          language={language}
          onToggleLanguage={toggleLanguage}
          onMailClick={() => setStatusMessage("3 unread messages from mentors")}
          onNotifyClick={() => setStatusMessage("6 calendar events scheduled today")}
        />

        {/* Body Content */}
        {activeTab === "dashboard" ? (
          <Panel
            direction="row"
            flexGrow={1}
            flexBasis={0}
            spacing={14}
            padding={0}
            alignItems="stretch"
          >
            {/* Left + Center Main Workspace */}
            <Panel
              direction="column"
              flexGrow={1}
              flexBasis={0}
              minWidth={440}
              spacing={10}
              padding={0}
              alignItems="stretch"
            >
              {/* Full-Width Top Greeting Banner */}
              <HeroBanner
                studentName="Grace"
                taskCount={3}
                onReview={() => {
                  setActiveTab("assessments");
                  setStatusMessage("Opened pending 3 tasks review");
                }}
              />

              {/* Bottom 2 Columns: (Performance + Linked Teachers) | (My Visit) */}
              <Panel
                direction="row"
                flexGrow={1}
                flexBasis={0}
                spacing={12}
                padding={0}
                alignItems="stretch"
              >
                {/* Left Column */}
                <Panel
                  direction="column"
                  flexGrow={1}
                  flexBasis={0}
                  minWidth={240}
                  spacing={10}
                  padding={0}
                  alignItems="stretch"
                >
                  <PerformanceCard
                    data={performanceData}
                    selectedMonth={month}
                    onToggleMonth={toggleMonth}
                    showAllLessons={showAllLessons}
                    onToggleAllLessons={() => setShowAllLessons((s) => !s)}
                  />

                  <LinkedTeachersCard
                    onSeeAll={() => setActiveTab("forum")}
                    onContactTeacher={(id, action) => {
                      const who = id === "t1" ? "Mary Johnson" : "James Brown";
                      setStatusMessage(`Opening ${action} with ${who}...`);
                    }}
                  />
                </Panel>

                {/* Right Column: My Visit */}
                <MyVisitCard
                  data={visitData}
                  selectedMonth={month}
                  onToggleMonth={toggleMonth}
                />
              </Panel>
            </Panel>

            {/* Right Rail: Calendar + Upcoming Events */}
            <Panel
              direction="column"
              width={248}
              minWidth={248}
              maxWidth={248}
              spacing={10}
              padding={0}
              alignItems="stretch"
            >
              <CalendarCard
                events={CALENDAR_EVENTS}
                activeEventId={activeEventId}
                onSelectEvent={(id) => {
                  setActiveEventId(id);
                  const found = CALENDAR_EVENTS.find((e) => e.id === id);
                  if (found) setStatusMessage(`Selected ${found.title} (${found.timeRange})`);
                }}
                selectedFilter={calendarFilter}
                onToggleFilter={() =>
                  setCalendarFilter((f) => (f === "Today" ? "Week" : "Today"))
                }
              />

              <UpcomingEventsCard
                onSeeAll={() => setActiveTab("schedule")}
                onEventClick={(id) => {
                  setStatusMessage(
                    id === "u1"
                      ? "Robot Fest details: 14 Dec 2023 @ 12:00 pm"
                      : "Minecraft Webinar details: 21 Dec 2023 @ 11:00 pm"
                  );
                }}
              />
            </Panel>
          </Panel>
        ) : (
          /* Lightweight Interactive View for Other Sidebar Tabs */
          <Panel
            direction="column"
            flexGrow={1}
            flexBasis={0}
            background="#FFFFFF"
            cornerRadius={16}
            padding={24}
            spacing={16}
            alignItems="start"
            justifyContent="start"
          >
            <Panel direction="row" alignItems="center" spacing={10} padding={0}>
              <Panel
                direction="row"
                alignItems="center"
                justifyContent="center"
                width={38}
                height={38}
                cornerRadius={12}
                background="#EEF2FF"
                padding={0}
              >
                <Icon name={activeTabInfo.icon} size={18} color="#3D509B" />
              </Panel>
              <Panel direction="column" padding={0} spacing={2}>
                <Label text={activeTabInfo.label} fontSize={18} bold={true} color="#1E2538" />
                <Label
                  text={`Grace Stanley · Active month: ${month} · Search filter: ${searchText || "None"}`}
                  fontSize={11}
                  color="#64748B"
                />
              </Panel>
            </Panel>

            <Label
              text={`You are viewing the ${activeTabInfo.label} workspace in Light Mode. Switch back to Dashboard anytime via the left rail or button below.`}
              fontSize={12}
              color="#475569"
            />

            <Button
              text="Back to Dashboard"
              fontSize={12}
              cornerRadius={12}
              background="#5F74E6"
              color="#FFFFFF"
              borderColor="#5F74E6"
              onClick={() => setActiveTab("dashboard")}
            />
          </Panel>
        )}

        {statusMessage ? (
          <Panel
            direction="row"
            height={20}
            padding={{ left: 10, right: 10 }}
            cornerRadius={8}
            background="#DCE2F2"
            alignItems="center"
            justifyContent="space-between"
          >
            <Label text={statusMessage} fontSize={10} color="#3D509B" />
            <Panel padding={0} onClick={() => setStatusMessage("")}>
              <Icon name="x" size={11} color="#3D509B" />
            </Panel>
          </Panel>
        ) : null}
      </Panel>
    </Panel>
  );
}

setWindowTitle("Dashboard for student - Light mode");
render(<StudentDashboardApp />);
