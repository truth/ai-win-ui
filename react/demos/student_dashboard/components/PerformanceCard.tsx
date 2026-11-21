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
    <Panel direction="column" padding={0} spacing={8} alignItems="stretch">
      {/* External Title Row */}
      <Panel
        direction="row"
        alignItems="center"
        justifyContent="space-between"
        height={22}
        padding={0}
      >
        <Label text="Performance" fontSize={13} bold={true} color="#1E2538" flexGrow={1} />
        <Panel
          direction="row"
          alignItems="center"
          justifyContent="center"
          width={84}
          minWidth={84}
          height={22}
          padding={{ left: 8, right: 6 }}
          cornerRadius={11}
          background="#DCE2F2"
          spacing={4}
          onClick={onToggleMonth}
        >
          <Label text={selectedMonth} fontSize={10} fontWeight={500} color="#3D509B" />
          <Icon name="chevron-down" size={10} color="#3D509B" />
        </Panel>
      </Panel>

      {/* White Card Body */}
      <Panel
        direction="column"
        background="#FFFFFF"
        cornerRadius={14}
        padding={{ left: 16, top: 12, right: 16, bottom: 12 }}
        spacing={10}
        alignItems="stretch"
      >
        {/* Best Lessons Header Row */}
        <Panel
          direction="row"
          alignItems="center"
          justifyContent="space-between"
          padding={0}
        >
          <Panel direction="column" flexGrow={1} flexBasis={0} padding={0} spacing={2}>
            <Label
              text="The best lessons:"
              fontSize={10}
              bold={true}
              color="#1E2538"
            />
            <Panel
              direction="row"
              alignItems="center"
              spacing={8}
              padding={0}
            >
              <Label text="95.4" fontSize={22} bold={true} color="#1E2538" />
              <Label
                text={"Introduction\nto programming"}
                fontSize={9.5}
                color="#64748B"
              />
            </Panel>
          </Panel>

          <Panel
            direction="row"
            alignItems="center"
            justifyContent="center"
            width={80}
            minWidth={80}
            height={24}
            padding={{ left: 10, right: 10 }}
            cornerRadius={12}
            background={showAllLessons ? "#EEF2FF" : "#FFFFFF"}
            borderColor="#475569"
            border={{ left: 1, top: 1, right: 1, bottom: 1 }}
            onClick={onToggleAllLessons}
          >
            <Label
              text={showAllLessons ? "Top 6" : "All lessons"}
              fontSize={10}
              fontWeight={600}
              color={showAllLessons ? "#3D509B" : "#1E2538"}
            />
          </Panel>
        </Panel>

        {/* 6 Dual-Tone Bars */}
        <Panel
          direction="row"
          justifyContent="space-between"
          alignItems="end"
          height={156}
          padding={{ top: 6 }}
          spacing={4}
        >
          {data.map((bar, idx) => {
            const totalH = Math.max(50, Math.min(104, Math.round(bar.score * 1.08)));
            const lowerH = Math.max(28, Math.round(totalH * 0.68));
            return (
              <Panel
                key={idx}
                direction="column"
                alignItems="center"
                flexGrow={1}
                flexBasis={0}
                padding={0}
                spacing={4}
              >
                <Label
                  text={bar.score.toFixed(1).replace(".", ",")}
                  fontSize={9}
                  fontWeight={600}
                  color="#1E2538"
                />

                {/* Bar Container (top light blue + bottom navy overlay) */}
                <Panel
                  direction="column"
                  justifyContent="end"
                  alignItems="stretch"
                  width={18}
                  height={104}
                  padding={0}
                >
                  <Panel
                    direction="column"
                    justifyContent="end"
                    width={18}
                    height={totalH}
                    cornerRadius={5}
                    background="#5F74E6"
                    padding={0}
                  >
                    <Panel
                      width={18}
                      height={lowerH}
                      cornerRadius={5}
                      background="#3D509B"
                      padding={0}
                    />
                  </Panel>
                </Panel>

                <Label
                  text={bar.shortName}
                  fontSize={8.5}
                  color="#64748B"
                  horizontalAlign="center"
                />
              </Panel>
            );
          })}
        </Panel>
      </Panel>
    </Panel>
  );
}
