import { createElement } from "../../../runtime/index.ts";
import { Panel, Label, Icon, Divider } from "../../../runtime/index.ts";
import type { CalendarEvent } from "../data/mockData.ts";

export interface CalendarCardProps {
  events: CalendarEvent[];
  activeEventId: string;
  onSelectEvent: (eventId: string) => void;
  selectedFilter?: string;
  onToggleFilter?: () => void;
}

export function CalendarCard({
  events,
  activeEventId,
  onSelectEvent,
  selectedFilter = "Today",
  onToggleFilter,
}: CalendarCardProps) {
  return (
    <Panel
      direction="column"
      background="#FFFFFF"
      cornerRadius={14}
      padding={{ left: 14, top: 14, right: 14, bottom: 12 }}
      spacing={10}
      alignItems="stretch"
    >
      {/* Header inside Calendar card */}
      <Panel
        direction="row"
        alignItems="start"
        justifyContent="space-between"
        padding={0}
      >
        <Panel direction="column" flexGrow={1} flexBasis={0} padding={0} spacing={2}>
          <Label text="Calendar" fontSize={14} bold={true} color="#1E2538" />
          <Label text="6 events today" fontSize={9.5} color="#8E9BAE" />
        </Panel>

        <Panel
          direction="row"
          alignItems="center"
          justifyContent="center"
          width={64}
          minWidth={64}
          height={22}
          padding={{ left: 8, right: 6 }}
          cornerRadius={11}
          background="#F1F3F9"
          spacing={4}
          onClick={onToggleFilter}
        >
          <Label text={selectedFilter} fontSize={10} color="#475569" />
          <Icon name="chevron-down" size={10} color="#475569" />
        </Panel>
      </Panel>

      {/* Timeline Slots */}
      <Panel direction="column" spacing={7} padding={0} alignItems="stretch">
        {events.map((ev, idx) => {
          const isActive = ev.id === activeEventId;
          return (
            <Panel key={ev.id} direction="column" padding={0} spacing={7} alignItems="stretch">
              <Panel
                direction="row"
                alignItems="center"
                padding={0}
                spacing={6}
              >
                {/* Left Time Column */}
                <Panel width={32} minWidth={32} padding={0}>
                  <Label
                    text={ev.timeLabel}
                    fontSize={9}
                    fontWeight={isActive ? 700 : 500}
                    color={isActive ? "#1E2538" : "#8E9BAE"}
                  />
                </Panel>

                {/* Vertical Accent Bar for Timeline */}
                <Panel
                  width={2.5}
                  height={44}
                  cornerRadius={1}
                  padding={0}
                  background={isActive ? "#FFFFFF" : "#3D509B"}
                />

                {/* Event Card */}
                <Panel
                  direction="row"
                  alignItems="center"
                  flexGrow={1}
                  flexBasis={0}
                  height={48}
                  padding={{ left: 10, top: 6, right: 10, bottom: 6 }}
                  cornerRadius={10}
                  background={isActive ? "#5F74E6" : "#F1F3F9"}
                  spacing={8}
                  onClick={() => onSelectEvent(ev.id)}
                >
                  <Panel
                    direction="row"
                    alignItems="center"
                    justifyContent="center"
                    width={24}
                    height={24}
                    cornerRadius={12}
                    padding={0}
                    background={isActive ? "#7D8FED" : "#DCE2F2"}
                  >
                    <Icon
                      name={ev.icon}
                      size={11}
                      color={isActive ? "#FFFFFF" : "#64748B"}
                    />
                  </Panel>

                  <Panel direction="column" padding={0} spacing={2} flexGrow={1} flexBasis={0}>
                    <Label
                      text={ev.title}
                      fontSize={10.5}
                      bold={true}
                      color={isActive ? "#FFFFFF" : "#1E2538"}
                      ellipsis={true}
                    />
                    <Panel direction="row" alignItems="center" spacing={4} padding={0}>
                      <Icon
                        name="clock"
                        size={9}
                        color={isActive ? "#DCE3F7" : "#8E9BAE"}
                      />
                      <Label
                        text={`${ev.timeRange}, ${ev.roomInfo}`}
                        fontSize={8.5}
                        color={isActive ? "#E2E8F0" : "#8E9BAE"}
                        ellipsis={true}
                      />
                    </Panel>
                  </Panel>
                </Panel>
              </Panel>

              {/* Dashed Current-Time Indicator right below first 10:00 card */}
              {idx === 0 ? (
                <Panel direction="row" alignItems="center" height={10} padding={0} spacing={4}>
                  <Panel
                    width={7}
                    height={7}
                    cornerRadius={3.5}
                    background="#3D509B"
                    padding={0}
                  />
                  <Panel flexGrow={1} padding={0}>
                    <Divider dashed={true} thickness={1} color="#3D509B" />
                  </Panel>
                </Panel>
              ) : null}
            </Panel>
          );
        })}

        {/* Bottom 14:30 Footer Mark */}
        <Panel direction="row" padding={{ left: 2 }}>
          <Label text="14:30" fontSize={9} color="#8E9BAE" />
        </Panel>
      </Panel>
    </Panel>
  );
}
