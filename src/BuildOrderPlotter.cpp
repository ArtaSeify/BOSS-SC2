#include "BuildOrderPlotter.h"
#include "BOSSConfig.h"

using namespace BOSS;

BuildOrderPlotter::BuildOrderPlotter()
{

}

void BuildOrderPlotter::setOutputDir(const std::string & dir)
{
    m_outputDir = dir;
}

void BuildOrderPlotter::addPlot(const std::string & name, const GameState & state, const BuildOrderAbilities & buildOrder)
{
    m_buildOrderNames.push_back(name);
    m_allPlots.push_back(BuildOrderPlotData(state, buildOrder));
}

void BuildOrderPlotter::doPlots()
{
    std::string buildOrderFilename = "AllBuildOrders.gpl";

    // if we only have one build order, name the file after it
    if (m_allPlots.size() == 1)
    {
        buildOrderFilename = m_buildOrderNames[0] + "_BuildOrder.gpl";
    }

    writeBuildOrderPlot(m_allPlots, m_outputDir + "/" + buildOrderFilename);
    
    // write resource plots
    for (size_t i(0); i < m_allPlots.size(); i++)
    {
        std::stringstream rss;
        rss << m_outputDir << "/" << m_buildOrderNames[i] << "_ResourcePlot.gpl";
        writeResourcePlot(m_allPlots[i], rss.str());
    }

    // write army plots
    for (size_t i(0); i < m_allPlots.size(); i++)
    {
        std::stringstream rss;
        rss << m_outputDir << "/" << m_buildOrderNames[i] << "_ArmyPlot.gpl";
        writeArmyValuePlot(m_allPlots[i], rss.str());
    }

    getPlotJSON(m_allPlots);
}

void BuildOrderPlotter::writeResourcePlot(const BuildOrderPlotData & plot, const std::string & filename)
{
    std::string noext = RemoveFileExtension(filename);
    std::stringstream mineralss;

    for (size_t i(0); i < plot.m_minerals.size(); ++i)
    {
        mineralss << plot.m_minerals[i].first << " " << plot.m_minerals[i].second << std::endl;
    }

    std::stringstream gasss;

    for (size_t i(0); i < plot.m_gas.size(); ++i)
    {
        gasss << plot.m_gas[i].first << " " << plot.m_gas[i].second << std::endl;
    }

    WriteGnuPlot(noext + "_minerals", mineralss.str(), " with lines ");
    WriteGnuPlot(noext + "_gas", gasss.str(), " with lines ");
}

void BuildOrderPlotter::writeBuildOrderPlot(const std::vector<BuildOrderPlotData> & plots, const std::string & filename)
{
    std::stringstream ss;
    int maxY = 0;
    for (size_t p(0); p < plots.size(); ++p)
    {
        maxY += (plots[p].m_maxLayer + 2) * (plots[p].m_boxHeight + plots[p].m_boxHeightBuffer) + 15;
    }

    //ss << "set title \"Title Goes Here\"" << std::endl;
    //ss << "set xlabel \"Time (frames)\"" << std::endl;
    ss << "set style rect fc lt -1 fs transparent solid 0.15" << std::endl;
    //ss << "set style rect fc lt -1 fs solid" << std::endl;
    ss << "set xrange [" << -(plots[0].m_maxFinishTime*.03) << ":" << 1.03*plots[0].m_maxFinishTime << "]" << std::endl;
    ss << "set yrange [-15:" << maxY << "]" << std::endl;
    ss << "unset ytics" << std::endl;
    ss << "set grid xtics" << std::endl;
    ss << "set nokey" << std::endl;
    //ss << "set size ratio " << (0.05 * m_maxLayer) << std::endl;
    ss << "set terminal wxt size 960,300" << std::endl;
    ss << "plotHeight = " << 1000 << std::endl;
    ss << "boxHeight = " << plots[0].m_boxHeight << std::endl;
    ss << "boxHeightBuffer = " << plots[0].m_boxHeightBuffer << std::endl;
    ss << "boxWidthScale = " << 1.0 << std::endl;
    
    int currentLayer = 0;
    int currentObject = plots[0].m_buildOrder.size();

    for (size_t p(0); p < plots.size(); ++p)
    {
        const BuildOrderAbilities & buildOrder = plots[p].m_buildOrder;

        for (size_t i(0); i < buildOrder.size(); ++i)
        {
            const Rectangle & rect = plots[p].m_rectangles[i];
            int rectWidth = (rect.bottomRight.x() - rect.topLeft.x());
            int rectCenterX = rect.bottomRight.x() - (rectWidth / 2);
        
            std::stringstream pos;

            pos << "(boxWidthScale * " << rectCenterX << "),";
            pos << "((boxHeight + boxHeightBuffer) * " << (plots[p].m_layers[i] + currentLayer) << " + boxHeight/2)";
            
            ss << "set object " << (currentObject+i+1) << " rect at ";
            ss << pos.str();
            ss << " size ";
            ss << "(boxWidthScale * " << (rectWidth) << "),";
            ss << "(boxHeight) ";
            //ss << "(boxWidthScale * " << m_finishTimes[i] << "),";
            //ss << "((boxHeight + boxHeightBuffer) * " << m_layers[i] << " + boxHeight) ";
            ss << "lw 1";

            ActionType type = buildOrder[i].first;
            if (type.isWorker())
            {
                ss << " fc rgb \"cyan\"";
            }
            else if (type.isSupplyProvider())
            {
                ss << " fc rgb \"gold\"";
            }
            else if (type.isRefinery())
            {
                ss << " fc rgb \"green\"";
            }
            else if (type.isBuilding())
            {
                ss << " fc rgb \"brown\"";
            }
            else if (type.isUpgrade())
            {
                ss << " fc rgb \"purple\"";
            }
            else if (type.isAbility())
            {
                if (type.getName() == "ChronoBoost")
                {
                    ss << " fs empty border lc rgb \"red\" front";
                }
            }

            ss << std::endl;

            if (type.isAbility())
            {
                if (type.getName() == "ChronoBoost")
                {
                    //ss << "set label " << (currentObject + i + 1) << " noenhanced at " << pos.str() << " \"" <<
                    //    "CB_" << buildOrder.getAbilityTargetType(i).getName() << "\" front center";
                    //ss << "set label " << (currentObject + i + 1) << " at " << pos.str() << " \"" << buildOrder[i].getName() << "\" front left";
                }
            }
            else
            {
                ss << "set label " << (currentObject + i + 1) << " at " << pos.str() << " \"" << type.getName() << "\" front center";
            }

            ss << std::endl;
        }

        currentLayer += plots[p].m_maxLayer + 2;
        currentObject += buildOrder.size();
    }

    ss << "plot -10000" << std::endl;

    std::ofstream out(filename);
    out << ss.str();
    out.close();
}

std::string BuildOrderPlotter::getPlotJSON(const std::vector<BuildOrderPlotData> & plots)
{
    std::stringstream ss;
    ss << "var plots = [\n";

    for (size_t p(0); p < plots.size(); ++p)
    {
        const BuildOrderAbilities & buildOrder = plots[p].m_buildOrder;
        
        ss << "{ name : \"" << m_buildOrderNames[p] << "\", buildOrder : [";

        for (size_t i(0); i < buildOrder.size(); ++i)
        {
            ActionType type = buildOrder[i].first;

            ss << "[\"" << type.getName() << "\", ";
            ss << plots[p].m_startTimes[i] << ", "; 
            ss << plots[p].m_finishTimes[i] << ", ";
            ss << plots[p].m_minerals[i].first << ", ";
            ss << plots[p].m_gas[i].first << ", ";
            ss << plots[p].m_layers[i] << ", ";
            
            if (type.isWorker())
            {
                ss << "\"rgb(217, 255, 255)\"";
            }
            else if (type.isSupplyProvider())
            {
                ss << "\"rgb(255, 249, 217)\"";
            }
            else if (type.isRefinery())
            {
                ss << "\"rgb(217, 255, 217)\"";
            }
            else if (type.isBuilding())
            {
                ss << "\"rgb(241, 223, 223)\"";
            }
            else if (type.isUpgrade())
            {
                ss << "\"rgb(255, 217, 255)\"";
            }
            else
            {
                ss << "\"rgb(190, 190, 190)\"";
            }
                        
            ss << "]";
            if (i < buildOrder.size() - 1) { ss << ", "; }
        }
        
        ss << "]}";
        if (p < plots.size() - 1) { ss << ", "; }
        ss << "\n";
    }

    ss << "]";

    return ss.str();
}

const std::vector<BuildOrderPlotData> & BuildOrderPlotter::getPlots() const
{
    return m_allPlots;
}

void BuildOrderPlotter::writeArmyValuePlot(const BuildOrderPlotData & plot, const std::string & filename)
{
    std::stringstream datass;
    for (size_t i(0); i < plot.m_buildOrder.size(); ++i)
    {
        datass << plot.m_startTimes[i] << " " << plot.m_armyValues[i] << std::endl;
    }
 
    WriteGnuPlot(filename, datass.str(), " with steps");
}


std::string BuildOrderPlotter::GetFileNameFromPath(const std::string & path)
{
    std::string temp(path);

    size_t last_slash_idx = temp.find_last_of("\\/");
    if (std::string::npos != last_slash_idx)
    {
        temp.erase(0, last_slash_idx + 1);
    }
    
    return temp;
}

std::string BuildOrderPlotter::RemoveFileExtension(const std::string & path)
{
    std::string temp(path);

    size_t period_idx = temp.rfind('.');
    if (std::string::npos != period_idx)
    {
        temp.erase(period_idx);
    }

    return temp;
}

void BuildOrderPlotter::WriteGnuPlot(const std::string & filename, const std::string & data, const std::string & args)
{
    std::string file = RemoveFileExtension(GetFileNameFromPath(filename));
    std::string noext = RemoveFileExtension(filename);

    std::ofstream dataout(noext + "_data.txt");
    dataout << data;
    dataout.close();

    std::stringstream ss;
    ss << "set xlabel \"Time (frames)\"" << std::endl;
    ss << "set ylabel \"Resource Over Time\"" << std::endl;
    ss << "plot \"" << (file + "_data.txt") << "\" " << args << std::endl;

    std::ofstream out(noext + ".gpl");
    out << ss.str();
    out.close();
}
