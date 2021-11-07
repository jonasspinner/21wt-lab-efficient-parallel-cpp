# Based on exercise1/hash/eval/make_plots.R

# Loads libraries
library(tidyverse)

# Path to the result files
res_folder='./build/'
# Different versions of the algorithm we want to plot
algos = c('std', 'a', 'b', 'c', 'd')

# Reads the files and creates the union of all rows
data <- algos %>% map(function(a) {
  # The name of the file with the results
  file <- paste(res_folder, 'pq_', a, '.txt', sep='')
  # Reads the data
  raw <- read.table(file, comment.char = '#', col.names = c('it', 'sec', 'deg', 'n_start', 'n_end', 'tinsert', 'tpop', 'errors'))
  # Adds an extra column with the algorithm name
  mutate(raw, algo = a)
}) %>% reduce(union_all)

# Makes x axis discrete
data$sec <- factor(data$sec)
# Re-orders the algorithms according to our chosen order, instead of alphabetically
data$algo <- factor(data$algo, levels = algos)

# Plots one of the 'time' columns
plot_time <- function(t) {

  # This plots the mean and standard error for each section, and connects the points with lines
  ggplot(data, aes(x = sec, y = {{t}}, group = algo, color = algo)) +
      stat_summary(fun.data = mean_se, geom = 'pointrange') +
      stat_summary(fun = mean, geom = 'line') +
      ylim(0,NA)

  # Boxplots can be used to show more details about the distributions
  #ggplot(data, aes(x = sec, y = {{t}}, color = algo)) +
  #  geom_boxplot()

  # If there are very few observations, you may instead want to simply plot all of them
  #ggplot(data, aes(x = sec, y = {{t}}, group = algo, color = algo)) +
  #  geom_point(position = position_jitter(width = 0.2, height = 0)) +
  #  stat_summary(fun = mean, geom = 'line')
}

# Opens a PDF to store the plots
pdf("plots.pdf", width=10, height=5)

# Plots the data
plot_time(tinsert)  + labs(title = 'Insertion',         x = 'Section', y = 'Time [ns]', color = 'Algorithm')
plot_time(tpop)     + labs(title = 'Pop',               x = 'Section', y = 'Time [ns]', color = 'Algorithm')

# Closes the PDF
dev.off()
