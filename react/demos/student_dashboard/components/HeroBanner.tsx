import { createElement } from "../../../runtime/index.ts";
import { Panel, Label, Image } from "../../../runtime/index.ts";

export interface HeroBannerProps {
  studentName?: string;
  taskCount?: number;
  onReview?: () => void;
}

export function HeroBanner({
  studentName = "Grace",
  taskCount = 3,
  onReview,
}: HeroBannerProps) {
  return (
    <Panel
      direction="row"
      alignItems="center"
      justifyContent="space-between"
      background="#FFFFFF"
      cornerRadius={14}
      padding={{ left: 22, top: 10, right: 18, bottom: 10 }}
      spacing={12}
      height={108}
      minHeight={108}
    >
      {/* Left Text Column */}
      <Panel
        direction="column"
        justifyContent="center"
        padding={0}
        spacing={0}
        flexGrow={1}
        flexBasis={0}
      >
        <Label
          text={`Hello ${studentName}!`}
          fontSize={18}
          bold={true}
          color="#1E2538"
        />
        <Panel height={5} padding={0} />
        <Label
          text={`You have ${taskCount} new tasks. It is a lot of work for today! So let's start!`}
          fontSize={11}
          color="#64748B"
        />
        <Panel height={8} padding={0} />
        <Panel
          direction="column"
          alignItems="start"
          padding={0}
          spacing={1}
          onClick={onReview}
        >
          <Label text="review it" fontSize={11} fontWeight={600} color="#4F63CE" />
          <Panel width={44} height={1} background="#4F63CE" padding={0} />
        </Panel>
      </Panel>

      {/* Right 3D Student Desk Illustration */}
      <Panel
        direction="row"
        alignItems="center"
        justifyContent="center"
        width={154}
        minWidth={154}
        height={96}
        padding={0}
      >
        <Image
          src="images/student_hero.png"
          width={150}
          height={94}
          stretch="uniform"
        />
      </Panel>
    </Panel>
  );
}
