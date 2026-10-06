#!/usr/bin/env python3
"""Build the report DOCX and A2 poster PDF from verified repository evidence.
Requires python-docx, reportlab and Pillow. Render the DOCX with the packaged
Documents renderer before exporting a report PDF or distributing these files.
"""
from pathlib import Path
import csv, math, os
from xml.sax.saxutils import escape
from docx import Document
from docx.shared import Inches, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_CELL_VERTICAL_ALIGNMENT
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from reportlab.pdfgen import canvas
from reportlab.lib.colors import HexColor, white
from reportlab.lib.pagesizes import A2, landscape
from reportlab.platypus import Paragraph
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.utils import ImageReader
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'output'; (OUT/'documents').mkdir(parents=True,exist_ok=True); (OUT/'pdf').mkdir(parents=True,exist_ok=True)
rows=list(csv.DictReader((ROOT/'benchmarks/results/mac.csv').open()))
cpu={int(r['grid']):r for r in rows if r['backend']=='cpu'}
metal={int(r['grid']):r for r in rows if r['backend']=='metal'}
speedup=lambda size: float(cpu[size]['total_ms'])/float(metal[size]['total_ms'])
report=Document(); sec=report.sections[0]
for border in list(report.styles.element.iter(qn('w:pBdr'))):
    border.getparent().remove(border)
sec.page_width=Inches(8.5); sec.page_height=Inches(11)
sec.top_margin=sec.bottom_margin=Inches(0.7); sec.left_margin=sec.right_margin=Inches(0.8)
normal=report.styles['Normal']; normal.font.name='Liberation Sans'; normal.font.size=Pt(11)
normal.paragraph_format.line_spacing=1.12; normal.paragraph_format.space_after=Pt(7)
for name,size in [('Title',28),('Heading 1',17),('Heading 2',13)]:
    style=report.styles[name]; style.font.name='Liberation Sans'; style.font.size=Pt(size); style.font.color.rgb=RGBColor(0,0,0)
    style.paragraph_format.space_before=Pt(9); style.paragraph_format.space_after=Pt(7)
    style.paragraph_format.keep_with_next=True
report.styles['Caption'].font.size=Pt(9); report.styles['Caption'].font.color.rgb=RGBColor.from_string('52616C')
report.styles['Caption'].paragraph_format.space_after=Pt(9)
report.styles['Caption'].paragraph_format.keep_with_next=False
report.styles['Subtitle'].font.color.rgb=RGBColor(0,0,0)
report.styles['Subtitle'].font.italic=False
footer=sec.footer.paragraphs[0]; footer.alignment=WD_ALIGN_PARAGRAPH.CENTER
r=footer.add_run('FireScope  |  '); r.font.size=Pt(9)
f=OxmlElement('w:fldSimple'); f.set(qn('w:instr'),'PAGE'); footer._p.append(f)
report.core_properties.title='FireScope wildfire simulation project report'
report.core_properties.subject='Cellular wildfire model GPU parallelism OpenGL graphics and mitigation comparison'
report.core_properties.author=''; report.core_properties.last_modified_by=''
source=[]
def para(text):
    report.add_paragraph(text); source.append(text+'\n')
def heading(number,title):
    report.add_heading(f'{number} {title}',level=1); source.append(f'## {number} {title}\n')
def page():
    p=report.add_page_break(); p.paragraph_format.keep_with_next=False
    p.paragraph_format.space_before=Pt(0); p.paragraph_format.space_after=Pt(0)
    p.paragraph_format.line_spacing=Pt(1); p.paragraph_format.widow_control=False
    source.append('\n---\n')
def picture(path,caption,width=6.7):
    p=report.add_paragraph(); p.paragraph_format.keep_with_next=True
    p.add_run().add_picture(str(ROOT/path),width=Inches(width))
    report.add_paragraph(caption,'Caption'); source.append(f'![{caption}]({os.path.relpath(ROOT/path,ROOT/"docs")})\n')
def table(headers,data,widths):
    t=report.add_table(rows=1,cols=len(headers)); t.alignment=WD_TABLE_ALIGNMENT.CENTER; t.autofit=False
    for c,w in zip(t.columns,widths): c.width=Inches(w)
    for cell,text in zip(t.rows[0].cells,headers): cell.text=text
    repeat=OxmlElement('w:tblHeader'); t.rows[0]._tr.get_or_add_trPr().append(repeat)
    for row in data:
        for cell,text in zip(t.add_row().cells,row): cell.text=str(text)
    for ri,row in enumerate(t.rows):
        no_split=OxmlElement('w:cantSplit'); row._tr.get_or_add_trPr().append(no_split)
        for ci,cell in enumerate(row.cells):
            cell.width=Inches(widths[ci]); cell.vertical_alignment=WD_CELL_VERTICAL_ALIGNMENT.CENTER
            pr=cell._tc.get_or_add_tcPr(); borders=OxmlElement('w:tcBorders')
            for edge in ('top','left','bottom','right'):
                e=OxmlElement('w:'+edge); e.set(qn('w:val'),'single'); e.set(qn('w:sz'),'4'); e.set(qn('w:color'),'D9D9D9'); borders.append(e)
            pr.append(borders); margins=OxmlElement('w:tcMar')
            for edge in ('top','left','bottom','right'):
                e=OxmlElement('w:'+edge);e.set(qn('w:w'),'85');e.set(qn('w:type'),'dxa');margins.append(e)
            pr.append(margins); shade=OxmlElement('w:shd');shade.set(qn('w:fill'),'19384A' if ri==0 else ('F2F6F8' if ri%2==0 else 'FFFFFF'));pr.append(shade)
            for p in cell.paragraphs:
                p.paragraph_format.space_after=Pt(2);p.paragraph_format.space_before=Pt(2)
                if ci>0: p.alignment=WD_ALIGN_PARAGRAPH.CENTER
                for r in p.runs:
                    r.font.size=Pt(10);r.bold=ri==0
                    if ri==0:r.font.color.rgb=RGBColor(255,255,255)
    source.append('| '+' | '.join(headers)+' |\n| '+' | '.join(['---']*len(headers))+' |\n'+''.join('| '+' | '.join(map(str,r))+' |\n' for r in data)+'\n')
# Native editable Word math; rendered QA verifies OMML preservation in LibreOffice.
def mr(text):
    r=OxmlElement('m:r'); pr=OxmlElement('m:rPr'); sty=OxmlElement('m:sty');sty.set(qn('m:val'),'p');pr.append(sty);r.append(pr)
    t=OxmlElement('m:t');t.text=text;r.append(t);return r

def script(base,sub=None,sup=None):
    name='m:sSubSup' if sub is not None and sup is not None else ('m:sSub' if sub is not None else 'm:sSup')
    e=OxmlElement(name);b=OxmlElement('m:e');b.append(mr(base));e.append(b)
    for tag,val in [('m:sub',sub),('m:sup',sup)]:
        if val is not None:
            a=OxmlElement(tag);a.append(mr(val) if isinstance(val,str) else val);e.append(a)
    return e

def equation(nodes,description):
    p=report.add_paragraph();p.alignment=WD_ALIGN_PARAGRAPH.CENTER
    eq=OxmlElement('m:oMath');
    for node in nodes: eq.append(mr(node) if isinstance(node,str) else node)
    p._p.append(eq); source.append(description+'\n')

report.add_paragraph('FireScope wildfire simulation project report','Title')
report.add_paragraph('CPU CUDA Metal and OpenGL',style='Subtitle')
para('FireScope combines a cellular wildfire model with interactive 3D graphics and paired mitigation experiments. This report describes the implementation, its verified behavior and measured performance. The Mac CPU and Metal backends pass the correctness suite; the CUDA source awaits compilation and validation on an NVIDIA computer.')
picture('docs/images/realistic-forest-fire.png','Figure 1  Seed 42 on a 128 by 128 grid at timestep 55 in the Forest view')
para('Implementation and measurements reviewed on 7 October 2026. The project is an educational prototype with uncalibrated model parameters.')
page()
heading(1,'Abstract')
para(f'FireScope demonstrates GPU parallelism through wildfire spread and mitigation. A two-dimensional grid stores cell state, fuel, moisture, elevation, vegetation, density and burn age. Synchronous updates combine the influence of eight neighboring cells with wind, slope and moisture, using a deterministic random hash. A shared interface supports serial CPU, Apple Metal and NVIDIA CUDA implementations. OpenGL presents the state as a terrain and forest with camera controls, picking, fire and smoke. On an Apple M5 Pro, the measured headless Metal step API was {speedup(512):.2f} times faster than the CPU step API at 512 by 512 cells for a batch of 500 timesteps. A seeded firebreak comparison reduced the burned-cell count by 26.8 percent. These observations concern the implemented educational model; CUDA validation and optimization results remain pending.')
heading(2,'Problem statement')
para('Wildfire spread depends on interactions among local fuel, environmental conditions and terrain. Evaluating every cell repeatedly becomes expensive as the grid grows. FireScope provides a controllable setting in which a student can inspect those interactions, compare compute backends and test a mitigation edit against the same initial scenario. The objective is a reproducible simulation and graphics project rather than an operational fire forecast.')
heading(3,'SDG relevance')
para('The project has an educational relationship to SDG 13 Climate Action and SDG 15 Life on Land [5, 6]. Environmental controls help explain hazard sensitivity; paired firebreak and wet-zone experiments illustrate how interventions can change outcomes within a simplified model. The work does not quantify real-world climate benefits, habitat preservation or emergency response effectiveness.')
heading(4,'Related work')
para('Alexandridis et al. describe a cellular model of forest fire spread that accounts for vegetation, wind and terrain [1]. FireScope implements a simplified derived formulation with explicit project constants; it does not reproduce the paper’s calibration, real incident data or spotting mechanism. The specification also identifies WildFire-Simulation-IPC as an algorithm reference [2] and CIS565-Final-Project as a graphics reference [3]. The latter uses branching forest geometry and a more complex combustion and smoke model. FireScope retains its cellular simulation and uses bounded billboard particles. The team-wildfire repository is an additional project reference [4].')
heading(5,'System architecture')
para('Scenario generation supplies the same structure-of-arrays grid to a backend selected through ISimulationBackend. The CPU, CUDA and Metal backends own computation and elapsed timesteps. The renderer receives a compact state and intensity payload once per frame. Interaction edits modify backend state; snapshots preserve the complete scenario. A separate headless path supports correctness checks and benchmark collection without OpenGL.')
page()
heading(6,'Cellular wildfire model')
para('Each unburned cell evaluates its eight Moore neighbors without wrapping across boundaries. Only neighbors currently burning contribute. The direction is measured from source to target; cardinal distance is the cell size and diagonal distance is the cell size multiplied by the square root of two. Slope is the arctangent of the elevation difference divided by that distance.')
equation([script('p','ij'),' = clamp[',script('p','0'),'(0.5 + ',script('v','i'),')(0.5 + ',script('d','i'),') ',script('e',sup=script('E','ij')),', 0, 1]'],'Neighbor probability uses base ignition, vegetation, density and an exponential environment factor.')
equation([script('E','ij'),' = ',script('k','w'),'u + ',script('k','a'),'u[cos(',script('θ','ij'),' − ',script('θ','w'),') − 1] + ',script('k','s'),script('s','ij'),' − ',script('k','m'),script('m','i','*')],'Environment exponent combines wind speed and alignment, terrain slope and effective moisture.')
equation([script('m','i','*'),' = 1 − (1 − ',script('m','i'),')(1 − H)'],'Effective moisture combines cell moisture and global humidity.')
# Native n-ary product with a lower limit.
nary=OxmlElement('m:nary');pr=OxmlElement('m:naryPr');ch=OxmlElement('m:chr');ch.set(qn('m:val'),'∏');pr.append(ch); hide=OxmlElement('m:supHide');hide.set(qn('m:val'),'1');pr.append(hide);nary.append(pr)
sub=OxmlElement('m:sub');sub.append(mr('j ∈ Bᵢ'));nary.append(sub);sup=OxmlElement('m:sup');nary.append(sup);e=OxmlElement('m:e');e.append(mr('(1 − '));e.append(script('p','ij'));e.append(mr(')'));nary.append(e)
equation([script('P','i'),' = 1 − ',nary],'Combined ignition probability is one minus the product of neighbor survival probabilities.')
para('The symbols denote base probability p0, vegetation v, density d, wind speed u, source-to-target angle, wind direction, slope s, cell moisture m and global humidity H. The coefficients kw, ka, ks and km are tunable project choices. The exponent is bounded to the interval from minus 80 to 80 before exponentiation. This improves numerical stability without providing physical calibration.')
para('Ignition compares an unsigned 32-bit hash of seed, cell coordinates and elapsed timestep with a 64-bit exclusive threshold equal to the floor of the total probability times 2 to the power 32. This preserves the probability-one boundary. Burning cells consume fuel every update and become Burned after fuel is exhausted and the minimum burn duration is reached.')
heading(7,'Data structures')
table(['Field','Representation','Purpose'],[['state','uint32','NoFuel Unburned Burning Burned'],['fuel','float','Remaining combustible fuel'],['moisture','float','Local moisture from 0 to 1'],['elevation','float','Terrain elevation in meters'],['vegetation and density','two floats','Ignition multipliers and forest distribution'],['burnTicks','uint32','Elapsed burning updates']], [1.9,1.4,3.4])
heading(8,'CPU implementation')
para('The serial CPU backend is the correctness reference. Every update reads the current grid and writes next state, fuel and burn age; only those evolving arrays swap after the entire grid is processed. Environmental fields remain in the current grid. This avoids an in-place update giving later cells access to newly ignited neighbors. State counts and average step timing are exposed through the common interface.')
page()
heading(9,'CUDA implementation')
para('The CUDA source implements a plain global-memory update kernel with one thread per cell and 16 by 16 blocks. Paired device buffers store state, fuel and burn age. Static fields include moisture, elevation, vegetation and density. Separate kernels apply brushes and compact the render payload into two bytes per cell. CUDA events expose device timing; synchronization supports the common synchronous interface.')
para('CUDA compilation, exact CPU parity, runtime behavior, memory checking and Windows portability have not yet been verified. The NVIDIA checklist requires a Release build, backend discovery, the full test suite, Compute Sanitizer checks and same-host CPU/CUDA benchmarks. A missing compiler or skipped GPU test must not be interpreted as success.')
para('Shared-memory tiling is the planned memory-hierarchy experiment. It must be evaluated after the global-memory kernel works on the NVIDIA machine. A tile with a one-cell halo can reuse neighboring state and elevation values, but loading overhead and synchronization may offset that reuse. No tiled speedup is claimed in this report.')
heading(10,'Metal implementation')
para('The Metal backend was developed after a standalone input-plus-one compute test passed. Objective-C++ manages shared Metal buffers, compute pipelines and command dispatch. One thread processes one cell, using separate current and next evolving fields and the same deterministic hash. A separate compact kernel generates the state and intensity payload. Separate compute encoders preserve tracked resource hazards across batched updates.')
para('Metal is verified on the Apple M5 Pro. Tests compare cell state, intensity and counts with CPU across deterministic cases and seeded wind, slope and moisture scenarios. GPU brush edits, reset, odd dimensions, batched updates and snapshots are covered. Floating-point probability evaluation can differ across devices, so a future NVIDIA result must be checked independently.')
table(['Backend','Implementation','Validation'],[['CPU','Serial C++ reference','Passed locally'],['Metal','Apple compute kernels','Passed locally'],['CUDA','Global-memory kernels','NVIDIA checks pending'],['OpenMP','Optional extension','Not implemented']], [1.0,2.5,3.2])
para('All nine current tests pass in the Mac CPU/Metal build. A dependency-free CPU-only build also passes its applicable tests; the GPU consistency test is intentionally skipped when no GPU backend is enabled. GPU access is required to count the Metal tests as verified.')
page()
heading(11,'OpenGL visualization')
picture('docs/images/realistic-forest-fire.png','Figure 2  Branching foliage terrain shadows flames and wind-driven smoke')
para('OpenGL 4.1 Core renders an indexed terrain with perspective transforms and depth testing. Shared corner heights are averaged from neighboring cell elevations. Picking intersects the same triangles used for display. Orbit and free-fly cameras support rotation, pan, zoom and movement; Forest view provides a closer orbit angle.')
para('The forest is an original instanced mesh of tapered trunks, branches, twigs and leaf cards. Sampled cells, jitter, scale, yaw and color variation reduce regular rows. A directional shadow map with a 3 by 3 filter shades terrain and alpha-cut foliage. Procedural soil, grass and rock detail, a sky gradient and distance haze improve depth cues. Burning foliage sheds as fuel decreases; burned cells leave charred branches.')
para('Fire and smoke use camera-facing quads with procedural opacity and color, sorted by camera depth. The renderer limits flames and glow to 2,048 instances each and smoke to 4,096 particles, with at most 500 smoke emissions per second. Smoke drifts upward and with wind. Visual motion does not change the cellular simulation or its random stream.')
page()
heading(12,'Interaction and mitigation')
picture('docs/images/mitigation-tools.png','Figure 3  A blue wet zone and a cleared patch created with the interaction tools')
para('Ignite starts fires in eligible cells. Firebreak sets cells to NoFuel. Water adds moisture up to one without directly extinguishing existing fire. Vegetation sets fuel, vegetation and density on NoFuel or Unburned cells; zero clears them and positive values regrow forest. It preserves Burning and Burned cells. Stroke interpolation fills gaps, and tree instances refresh after vegetation edits.')
para('The moisture overlay tints ground and foliage blue according to stored moisture. The inspector shows coordinates, state, fuel, moisture, elevation, vegetation, density, burn age, burning neighbors and combined ignition probability. Observed steps capture the selected cell’s previous probability. Full inspector readbacks add overhead and are excluded from the headless benchmark path.')
para('Capture stores all cell fields, configuration, environmental controls and elapsed timestep. A comparison runs independent baseline and edited scenarios for the same number of steps with the same seed and logical start time. Restore permits another intervention from the same source. Snapshots are held in memory; resetting or switching grid/backend discards them.')
page()
heading(13,'Experimental methodology')
para('The measured host is an Apple M5 Pro running macOS 27.0 on arm64, using a Release build with AppleClang 21. Each grid uses seed 42, the same generated forest, default environment and a central ignition of radius two. Sizes are 128, 256, 512 and 1024 cells per side, with 500 timesteps each. One warm-up precedes five measured runs; the median is reported. The scenario is restored before every run.')
para('Timing covers synchronous headless step batch wall time. Initialization, shader/pipeline creation, rendering and final GPU readback are excluded. CPU count scanning occurs inside its step API; GPU counts are produced during later compact readback. Consequently these measurements compare the implemented step APIs and do not represent isolated kernel arithmetic or end-to-end viewer speed. All 500 updates continue even if fire ends early.')
heading(14,'Performance results')
data=[]
for n in sorted(cpu):data.append([f'{n} by {n}',f"{float(cpu[n]['total_ms']):.3f}",f"{float(metal[n]['total_ms']):.3f}",f'{speedup(n):.2f} times',cpu[n]['burned_final']])
table(['Grid','CPU ms','Metal ms','Speedup','Burned final'],data,[1.25,1.2,1.2,1.25,1.8])
para('Final burned counts agree between CPU and Metal at every measured size. The growing speedup is consistent with larger workloads better amortizing GPU dispatch overhead, but these measurements alone do not isolate the cause. Per-step parity tests provide separate correctness evidence. Hardware, scene and batch length constrain how widely the performance result can be generalized.')
picture('benchmarks/results/mac-plots/speedup.png','Figure 4  Same-host median CPU time divided by Metal time for 500-step batches',width=5.4)
page()
heading(15,'Scenario comparison')
para('The seeded 128 by 128 demo compares a captured scenario with a north-south firebreak east of the central ignition. Both branches run for 500 updates from the same captured timestep and environment. The observed baseline burned 16,383 cells and the mitigated run burned 11,985 cells, a reduction of 4,398 cells or 26.8 percent.')
table(['Metric','Baseline','With firebreak'],[['Burned cells','16,383','11,985'],['Cleared NoFuel cells','0','728'],['Unburned cells','1','3,671']], [3.0,1.8,1.9])
para('The reduction includes land cleared for the barrier and should not all be described as preserved forest. The counts describe a simplified, single-seed experiment. A separate deterministic flat-grid test checks a known complete barrier: 651 baseline burned cells versus 315 mitigated burned cells, 21 cleared cells and 315 unburned cells on a 31 by 21 grid. These paired counts pass on CPU and Metal.')
heading(16,'Limitations')
para('The model is uncalibrated and uses synthetic terrain and simplified fuel categories. It omits spotting, certified real-weather inputs, real incident calibration and coupled fluid dynamics. Visual smoke and tree geometry are representative effects. Grid spacing has physical units, but an update is a logical model step and has no calibrated duration in seconds. The application is unsuitable for operational wildfire decisions.')
para('CUDA and Windows validation, shared-memory experiments and external-display checks remain incomplete. The recorded Mac benchmark excludes rendering and final GPU readback, and its CPU/GPU count-handling asymmetry must be preserved in result descriptions. In-memory snapshots do not yet provide persistent scenario files. Optional OpenMP is not implemented.')
heading(17,'Future work')
para('The next required experiment is NVIDIA validation, followed by same-host CPU/CUDA measurements and a global-memory versus shared-memory kernel comparison. Additional work includes repeated demo checks, projector scaling and a backup recording. Useful extensions include persistent scenario files, multiple seeds with confidence intervals, calibrated fuel/weather data and more realistic smoke, while retaining a clear distinction between simulation and visual effects.')
heading(18,'Conclusion')
para('FireScope provides a working CPU/Metal implementation, an OpenGL forest viewer and reproducible mitigation comparisons. The verified Mac tests support the model and interaction behavior, and the measurements show a 45.31 times step API speedup at 512 by 512 cells for the stated workload. The final project evaluation must add NVIDIA evidence and distinguish computational correctness from physical forecasting accuracy.')
page()
heading(19,'References')
refs=[
'[1] Alexandridis A, Vakalis D, Siettos C I and Bafas G V (2008). A cellular automata model for forest fire spread prediction: The case of the wildfire that swept through Spetses Island in 1990. Applied Mathematics and Computation 204(1), 191-201. https://doi.org/10.1016/j.amc.2008.06.046',
'[2] Vinnu124. WildFire-Simulation-IPC. Repository reference identified by the project specification. https://github.com/Vinnu124/WildFire-Simulation-IPC',
'[3] Mansur A, Lee S and Smith L. Wildfire Simulation and Rendering. CIS565-Final-Project. https://github.com/anthonymansur/CIS565-Final-Project',
'[4] herewegoblueno. team-wildfire. Repository reference identified by the project specification. https://github.com/herewegoblueno/team-wildfire',
'[5] United Nations Department of Economic and Social Affairs. Goal 13 Climate Action. https://sdgs.un.org/goals/goal13',
'[6] United Nations Department of Economic and Social Affairs. Goal 15 Life on Land. https://sdgs.un.org/goals/goal15',
'[7] FireScope implementation. docs/PROJECT_SPEC.md, docs/model.md and docs/architecture.md. Source and tests in this repository, reviewed 7 October 2026.',
'[8] FireScope measurements. benchmarks/results/mac.csv and mac.json; docs/PERFORMANCE.md. One warm-up, five measured runs, seed 42, 500 updates. Graphs in benchmarks/results/mac-plots.',
'[9] Dependency versions and attribution. GLFW 3.4, GLM 1.0.1 and Dear ImGui v1.91.9b. See cmake/Dependencies.cmake and THIRD_PARTY.md in this repository.']
for ref in refs:para(ref)
report.save(OUT/'documents/FireScope_Report.docx')
(ROOT/'docs/REPORT.md').write_text('# FireScope wildfire simulation project report\n\n'+ '\n'.join(source))

# Landscape A2 poster. All numbers are generated from the committed CSV or known demo evidence.
fontdir=Path(os.environ.get('FIRESCOPE_FONT_DIR', '/Users/harshit/.cache/codex-runtimes/codex-primary-runtime/dependencies/native/libreoffice-headless/libreoffice/LibreOfficeDev.app/Contents/Resources/fonts/truetype'))
if not fontdir.exists():
    raise RuntimeError('Set FIRESCOPE_FONT_DIR to a directory containing LiberationSans-Regular.ttf and LiberationSans-Bold.ttf')
pdfmetrics.registerFont(TTFont('FSRegular',str(fontdir/'LiberationSans-Regular.ttf')))
pdfmetrics.registerFont(TTFont('FSBold',str(fontdir/'LiberationSans-Bold.ttf')))
pdfmetrics.registerFontFamily('FSRegular',normal='FSRegular',bold='FSBold',italic='FSRegular',boldItalic='FSBold')
W,H=landscape(A2); c=canvas.Canvas(str(OUT/'pdf/FireScope_Poster.pdf'),pagesize=(W,H))
c.setTitle('FireScope wildfire simulation and mitigation');c.setAuthor('')
ink=HexColor('#173B36'); gray=HexColor('#526466'); accent=HexColor('#E78932')
c.setFillColor(HexColor('#F7FAF7'));c.rect(0,0,W,H,fill=1,stroke=0)
def text(x,y,value,size=20,bold=False,color=ink):
    c.setFillColor(color);c.setFont('FSBold' if bold else 'FSRegular',size);c.drawString(x,y,value)
def block(x,y,title,body,width,size=20):
    text(x,y,title,27,True); style=ParagraphStyle('body',fontName='FSRegular',fontSize=size,leading=size*1.28,textColor=ink)
    p=Paragraph(body,style);_,h=p.wrap(width,1000);p.drawOn(c,x,y-18-h);return y-18-h-24

def image(path,x,y,width):
    im=Image.open(ROOT/path); height=width*im.height/im.width
    c.drawImage(ImageReader(im),x,y-height,width=width,height=height,mask='auto');return height
text(55,H-87,'FireScope',64,True)
text(57,H-128,'Interactive wildfire simulation and mitigation',30)
text(58,H-163,'CPU and Metal verified on Mac  |  CUDA validation pending',20,color=gray)
left,right,col=58,876,750
hero_top=H-198
image('docs/images/realistic-forest-fire.png',left,hero_top,col)
text(left,hero_top-col*1664/2360-27,'Branching forest, terrain shadows, fire and wind-driven smoke',18,color=gray)
text(right,hero_top-43,f'{speedup(512):.2f}x',54,True)
text(right+238,hero_top-30,'Metal step API speedup',22,True)
text(right+238,hero_top-58,'512 by 512 cells, 500 updates',18,color=gray)
image('benchmarks/results/mac-plots/speedup.png',right+35,hero_top-92,650)
text(right,hero_top-520,'Apple M5 Pro  |  Median of five measured runs after one warm-up',16,color=gray)
text(right,hero_top-545,'Headless step batch time; rendering and final GPU readback excluded.',16,color=gray)
text(right,hero_top-570,'CPU counts are timed inside step; GPU counts occur during readback.',16,color=gray)
# Bottom three columns follow the MD poster outline.
y=H-795; cw=485
n=block(58,y,'Problem and objective','Explain how fuel, wind, moisture and slope affect fire spread while comparing CPU and GPU computation. Support interactive, reproducible mitigation experiments.',cw)
n=block(58,n,'Cellular model','Eight neighbors per cell. Ignition combines wind, slope, vegetation, density and moisture. Double buffers preserve synchronous updates; a seeded hash makes replay reproducible.',cw)
n=block(596,y,'Architecture','Scenario and controls -> CPU / CUDA / Metal -> compact state -> OpenGL terrain, forest and particles.',cw)
n=block(596,n,'Parallel computation','One GPU thread per cell with 16 by 16 groups. Nine Mac tests pass. CUDA and shared-memory optimization are awaiting NVIDIA validation; no CUDA speedup is claimed.',cw)
n=block(1134,y,'Mitigation comparison','A seeded 128 by 128 firebreak demo burned <b>11,985</b> cells versus <b>16,383</b> without intervention: <b>26.8% fewer</b>. Includes 728 cleared cells; 3,671 remain unburned.',cw)
n=block(1134,n,'SDGs and limitations','Educational alignment with SDG 13 Climate Action and SDG 15 Life on Land. Uncalibrated model with synthetic terrain; unsuitable for operational forecasts.',cw)
text(58,36,'Model reference: Alexandridis et al. (2008), DOI 10.1016/j.amc.2008.06.046. Full sources and methods are in the project report.',13,color=gray)
c.linkURL('https://doi.org/10.1016/j.amc.2008.06.046',(58,28,900,50),relative=0)
c.showPage();c.save()
print('Created report DOCX and A2 landscape poster PDF')
