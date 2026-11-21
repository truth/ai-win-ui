import { createElement } from "../../../runtime/index.ts";
import { Panel, Label, Icon } from "../../../runtime/index.ts";
import type { PerformanceBar } from "../data/mockData.ts";

export interface PerformanceCardProps {
  data: PerformanceBar[];
  selectedMonth: string;
  onToggleMonth: () => void;
  showAllLessons?: boolean;
  onToggleAllLessons?: () => void;
}

export function PerformanceCard({
  data,
  selectedMonth,
  onToggleMonth,
  showAllLessons,
  onToggleAllLessons,
}: PerformanceCardProps) {
  return (
    <Panel
      direction="column"
      background="#FFFFFF"
      cornerRadius={18}
      padding={[20, 16, 20, 16]}
      borderColor="#E2E8F0"
      border={[1, 1, 1, 1]}
      flexGrow={1}
    >
      {/* Title & Month Dropdown */}
      <Panel
        direction="row"
        alignItems="center"
        justifyContent="space-between"
        padding={0}
      >
        <Label text="Performance" fontSize={16} bold={true} color="#0F172A" />
        <Panel
          direction="row"
          alignItems="center"
          padding={[10, 4, 10, 4]}
          cornerRadius={12}
          background="#F8FAFC"
          borderColor="#E2E8F0"
          border={[1, 1, 1, 1]}
          spacing={4}
          onClick={onToggleMonth}
        >
          <Label text={selectedMonth} fontSize={11} color="#64748B" />
          <Icon name="chevron-down" size={12} color="#64748B" />
        </Panel>
      </Panel>

      <Panel height={12} padding={0} />

      {/* Summary Row */}
      <Panel
        direction="row"
        alignItems="center"
        justifyContent="space-between"
        padding={0}
      >
        <Panel direction="column" padding={0}>
          <Label text="The best lessons:" fontSize={11} color="#64748B" />
          <Panel direction="row" alignItems="baseline" spacing={6} padding={0}>
            <Label text="95.4" fontSize={22} bold={true} color="#0F172A" />
            <Label text="Introduction to programming" fontSize={10} color="#64748B" />
          </Panel>
        </Panel>

        <Panel
          direction="row"
          alignItems="center"
          padding={[12, 6, 12, 6]}
          cornerRadius={14}
          background={showAllLessons ? "#EEF2FF" : "#FFFFFF"}
          borderColor="#E2E8F0"
          border={[1, 1, 1, 1]}
          onClick={onToggleAllLessons}
        >
          <Label
            text="All lessons"
            fontSize={11}
            bold={true}
            color={showAllLessons ? "#3751FE" : "#0F172A"}
          />
        </Panel>
      </Panel>

      <Panel height={16} padding={0} />

      {/* 6-Column Bar Chart */}
      <Panel
        direction="row"
        justifyContent="space-between"
        alignItems="end"
        height={165}
        padding={[8, 0, 8, 0]}
      >
        {data.map((bar, idx) => {
          const barHeight = Math.max(12, Math.min(100, Math.round(bar.score)));
          return (
            <Panel
              key={idx}
              direction="column"
              alignItems="center"
              width={56}
              padding={0}
            >
              {/* Score text */}
              <Label
                text={bar.score.toFixed(1)}
                fontSize={10}
                bold={true}
                color="#475569"
              />
              <Panel height={6} padding={0} />

              {/* Vertical pill container (Total height 100) */}
              <Panel
                direction="column"
                justifyContent="end"
                width={22}
                height={100}
                cornerRadius={11}
                background="#E0E7FF"
                padding={0}
              >
                {/* Filled portion */}
                <Panel
                  width={22}
                  height={barHeight}
                  cornerRadius={11}
                  background="#3751FE"
                  padding={0}
                />
              </Panel>

              <Panel height={8} padding={0} />

              {/* Subject Label */}
              <Label
                text={bar.shortName}
                fontSize={9}
                color="#94A3B8"
                horizontalAlign="center"
              />
            </Panel>
          );
        })}
      </Panel>
    </Panel>
  );
}

